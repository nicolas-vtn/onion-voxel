#include "BlockRegistry.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>

// ---------------------------------------------------------------------------
// Anonymous namespace — file-local helpers
// ---------------------------------------------------------------------------
namespace
{
	static std::string ToFilename(const std::string& texturePath)
	{
		return texturePath + ".png";
	}

	static std::string ResolveTexture(const std::string& ref, const onion::voxel::BlockModel::Textures& t)
	{
		std::function<std::string(const std::string&)> resolve = [&](const std::string& r) -> std::string
		{
			if (!r.starts_with('#'))
				return ToFilename(r);
			const std::string key = r.substr(1);
			auto it = t.find(key);
			if (it != t.end())
				return resolve(it->second);
			return "";
		};
		return resolve(ref);
	}

	static onion::voxel::Face ToFace(const std::string& name)
	{
		using namespace onion::voxel;
		if (name == "up")
			return Face::Top;
		if (name == "down")
			return Face::Bottom;
		if (name == "north")
			return Face::Front;
		if (name == "south")
			return Face::Back;
		if (name == "west")
			return Face::Left;
		if (name == "east")
			return Face::Right;
		throw std::runtime_error("Unknown face: " + name);
	}

	static bool IsOverlayElem(const onion::voxel::BlockModel::Element& elem)
	{
		for (const auto& [faceName, face] : elem.Faces)
			if (face.Texture != "#overlay")
				return false;
		return true;
	}

	// ---------------------------------------------------------------------------
	// Geometry rotation helpers (for baking blockstate x/y rotations)
	// ---------------------------------------------------------------------------

	// Rotate a point 90° CW around center 8 on the X axis (looking from +X).
	// Cycle: Up → South → Down → North → Up
	static glm::vec3 RotateX90(glm::vec3 p)
	{
		float y = p.y - 8;
		float z = p.z - 8;
		return {p.x, z + 8, -y + 8};
	}

	// Rotate a point 90° CW around center 8 on the Y axis (looking down).
	// Cycle: North(-Z) → East(+X) → South(+Z) → West(-X) → North
	static glm::vec3 RotateY90(glm::vec3 p)
	{
		float x = p.x - 8;
		float z = p.z - 8;
		return {-z + 8, p.y, x + 8};
	}

	static glm::vec3 RotatePoint(glm::vec3 p, int steps, bool aroundY)
	{
		steps = ((steps % 4) + 4) % 4;
		for (int i = 0; i < steps; i++)
			p = aroundY ? RotateY90(p) : RotateX90(p);
		return p;
	}

	// Remap face names for Y-axis rotation (CW from above):
	// north(0) → east(1) → south(2) → west(3) → north
	static std::string RotateFaceY(const std::string& face, int steps)
	{
		static const char* cycle[4] = {"north", "east", "south", "west"};
		int idx = -1;
		if (face == "north")
			idx = 0;
		else if (face == "east")
			idx = 1;
		else if (face == "south")
			idx = 2;
		else if (face == "west")
			idx = 3;
		if (idx < 0)
			return face; // up / down unaffected
		return cycle[((idx + steps) % 4 + 4) % 4];
	}

	// Remap face names for X-axis rotation (CW from +X):
	// up(0) → south(1) → down(2) → north(3) → up
	static std::string RotateFaceX(const std::string& face, int steps)
	{
		static const char* cycle[4] = {"up", "south", "down", "north"};
		int idx = -1;
		if (face == "up")
			idx = 0;
		else if (face == "south")
			idx = 1;
		else if (face == "down")
			idx = 2;
		else if (face == "north")
			idx = 3;
		if (idx < 0)
			return face; // east / west unaffected
		return cycle[((idx + steps) % 4 + 4) % 4];
	}

	// Apply X then Y rotation to a BlockModel's element geometry and face name keys.
	// stepsX / stepsY = rotation_degrees / 90
	static onion::voxel::BlockModel ApplyModelRotation(onion::voxel::BlockModel model, int stepsX, int stepsY)
	{
		if (stepsX == 0 && stepsY == 0)
			return model;

		for (auto& elem : model.Elements)
		{
			glm::vec3 from(elem.From[0], elem.From[1], elem.From[2]);
			glm::vec3 to(elem.To[0], elem.To[1], elem.To[2]);

			// Apply X rotation first, then Y
			from = RotatePoint(from, stepsX, false);
			to = RotatePoint(to, stepsX, false);
			from = RotatePoint(from, stepsY, true);
			to = RotatePoint(to, stepsY, true);

			// Re-normalise min/max after rotation
			elem.From = {std::min(from.x, to.x), std::min(from.y, to.y), std::min(from.z, to.z)};
			elem.To   = {std::max(from.x, to.x), std::max(from.y, to.y), std::max(from.z, to.z)};

			// Remap face direction keys
			std::unordered_map<std::string, onion::voxel::BlockModel::Face> rotatedFaces;
			for (auto& [faceName, face] : elem.Faces)
			{
				std::string newName = faceName;
				if (stepsX != 0)
					newName = RotateFaceX(newName, stepsX);
				if (stepsY != 0)
					newName = RotateFaceY(newName, stepsY);
				rotatedFaces[newName] = std::move(face);
			}
			elem.Faces = std::move(rotatedFaces);
		}

		return model;
	}

} // anonymous namespace

// ---------------------------------------------------------------------------
// onion::voxel::BlockRegistry implementation
// ---------------------------------------------------------------------------
namespace onion::voxel
{
	BlockRegistry::BlockRegistry(std::shared_ptr<TextureAtlas> atlas) : m_Atlas(atlas) {}

	// ---- Direct texture-array registration (used by special-case blocks) ----
	void BlockRegistry::RegisterModel(BlockId id,
									  const std::vector<TextureInfo>& textures,
									  Model textureModel,
									  size_t variantIndex)
	{
		for (const auto& texture : textures)
			if (!texture.name.empty())
				m_AllTextureNames.insert(texture.name);

		PreRegistration reg;
		reg.id = id;
		reg.variantIndex = static_cast<uint8_t>(variantIndex);
		reg.textures = textures;
		reg.textureModel = textureModel;
		m_Registrations.push_back(std::move(reg));
	}

	void BlockRegistry::RegisterModelOverlay(BlockId id, const std::vector<TextureInfo>& textures, size_t variantIndex)
	{
		for (const auto& texture : textures)
			if (!texture.name.empty())
				m_AllTextureNames.insert(texture.name);

		PreOverlayRegistration reg;
		reg.id = id;
		reg.variantIndex = static_cast<uint8_t>(variantIndex);
		reg.textures = textures;

		m_RegistrationsOverlays.push_back(std::move(reg));
	}

	// ---- Register one VariantModel for a block ----
	void BlockRegistry::RegisterVariant(BlockId id, const VariantModel& variant, size_t variantIndex)
	{
		BlockModel blockModel = variant.Model;

		// ----- Special-case magic -----
		if (id == BlockId::Water)
		{
			BlockModel stoneModel = BlockModel::FromFile("stone.json");
			stoneModel.ModelTextures["all"] = blockModel.ModelTextures["particle"];
			blockModel = stoneModel;
			for (auto& elem : blockModel.Elements)
				for (auto& [fn, face] : elem.Faces)
					face.TintIndex = 1; // Water tint
		}
		else if (id == BlockId::Lava)
		{
			BlockModel stoneModel = BlockModel::FromFile("stone.json");
			stoneModel.ModelTextures["all"] = blockModel.ModelTextures["particle"];
			blockModel = stoneModel;
		}

		Model textureModel = Model::Block;
		if (!blockModel.ModelTextures["cross"].empty())
		{
			textureModel = Model::Cross;
			BlockModel stoneModel = BlockModel::FromFile("stone.json");
			stoneModel.ModelTextures["all"] = blockModel.ModelTextures["cross"];
			blockModel = stoneModel;
			if (id == BlockId::ShortGrass)
				for (auto& elem : blockModel.Elements)
					for (auto& [fn, face] : elem.Faces)
						face.TintIndex = 0; // Grass tint
		}

		if (blockModel.Elements.empty())
		{
			std::cout << "Warning: BlockModel for block ID " << static_cast<uint16_t>(id) << " ("
					  << BlockIds::GetName(id) << ") has no elements." << std::endl;
			blockModel = BlockModel::FromFile("bubble_coral_block.json");
		}

		// ----- Bake blockstate rotation into element geometry -----
		const int stepsX = ((variant.RotationX / 90) % 4 + 4) % 4;
		const int stepsY = ((variant.RotationY / 90) % 4 + 4) % 4;
		if (stepsX != 0 || stepsY != 0)
			blockModel = ApplyModelRotation(std::move(blockModel), stepsX, stepsY);

		// ----- Convert elements to TextureInfo -----
		std::vector<TextureInfo> baseTextures;
		baseTextures.reserve(6);
		bool baseInitialized = false;

		std::vector<TextureInfo> overlayTextures;
		overlayTextures.reserve(6);
		bool overlayInitialized = false;

		for (const auto& elem : blockModel.Elements)
		{
			bool isOverlay = IsOverlayElem(elem);

			for (const auto& [faceName, face] : elem.Faces)
			{
				Face f = ToFace(faceName);

				std::string resolved = ResolveTexture(face.Texture, blockModel.ModelTextures);
				if (resolved.empty())
					throw std::runtime_error("Failed to resolve texture reference: " + face.Texture);

				Tint tint = Tint::None;
				if (face.TintIndex.has_value())
				{
					if (face.TintIndex.value() == 0)
						tint = Tint::Grass;
					else if (face.TintIndex.value() == 1)
						tint = Tint::Water;
				}

				glm::u8vec3 from = {elem.From[0], elem.From[1], elem.From[2]};
				glm::u8vec3 to = {elem.To[0], elem.To[1], elem.To[2]};

				TextureInfo info{resolved, f, tint, from, to, face.UV};

				if (!isOverlay)
				{
					baseTextures.push_back(std::move(info));
					baseInitialized = true;
				}
				else
				{
					overlayTextures.push_back(std::move(info));
					overlayInitialized = true;
				}
			}
		}

		if (baseInitialized)
			RegisterModel(id, baseTextures, textureModel, variantIndex);

		if (overlayInitialized)
			RegisterModelOverlay(id, overlayTextures, variantIndex);
	}

	// ---- Register all variants parsed from a blockstate JSON ----
	void BlockRegistry::RegisterModel(BlockId id)
	{
		const auto& blockstates = BlockstateRegistry::Get();

		const auto it = blockstates.find(id);
		if (it == blockstates.end())
		{
			std::cout << "Warning: No blockstate found for block ID " << static_cast<uint16_t>(id) << " ("
					  << BlockIds::GetName(id) << ")." << std::endl;

			VariantModel defaultVariant;
			RegisterVariant(id, defaultVariant, 0);

			return;

			//throw std::runtime_error("BlockRegistry::RegisterModel: No blockstate found for block ID " +
			//						 std::to_string(static_cast<uint16_t>(id)));
		}

		const auto& variants = it->second;

		for (int i = 0; i < variants.size(); i++)
			RegisterVariant(id, variants[i], i);
	}

	// ---- Finalize: resolve TextureIDs from atlas ----
	void BlockRegistry::Register(BlockId id,
								 uint8_t variantIndex,
								 const std::vector<TextureInfo>& textures,
								 Model textureModel)
	{
		BlockTextures tex;
		tex.textureModel = textureModel;

		for (const auto& textureInfo : textures)
		{
			const std::string& textureName = textureInfo.name;

			if (textureName.empty())
				continue;

			FaceTexture faceTex;

			faceTex.texture = m_Atlas->GetTextureID(textureName);
			faceTex.face = textureInfo.face;
			faceTex.tintType = textureInfo.tintType;
			faceTex.textureType = m_Atlas->GetTextureTransparency(textureName);
			faceTex.from = textureInfo.from;
			faceTex.to = textureInfo.to;
			faceTex.uv = textureInfo.uv;

			tex.faces.push_back(std::move(faceTex));

			m_AllTextureNames.insert(textureName);
		}

		auto& variants = m_Blocks[id];
		// Grow the vector to fit this variant index
		if (variantIndex >= variants.size())
			variants.resize(variantIndex + 1);
		variants[variantIndex] = tex;
	}

	void BlockRegistry::RegisterOverlay(BlockId id, uint8_t variantIndex, const std::vector<TextureInfo>& textures)
	{
		auto it = m_Blocks.find(id);
		if (it == m_Blocks.end() || it->second.empty())
		{
			throw std::runtime_error("BlockRegistry::SetOverlay: Block ID not found: " +
									 std::to_string(static_cast<uint16_t>(id)));
		}

		// Extract the corresponding variant
		BlockTextures& blockTex = it->second[variantIndex];

		for (const auto& texture : textures)
		{
			FaceTexture overlayTex;

			overlayTex.face = texture.face;
			overlayTex.texture = m_Atlas->GetTextureID(texture.name);
			overlayTex.tintType = texture.tintType;
			overlayTex.textureType = m_Atlas->GetTextureTransparency(texture.name);
			overlayTex.from = texture.from;
			overlayTex.to = texture.to;
			overlayTex.uv = texture.uv;

			blockTex.overlay.push_back(std::move(overlayTex));

			m_AllTextureNames.insert(texture.name);
		}
	}

	void BlockRegistry::Initialize()
	{
		ReloadTextures();
	}

	void BlockRegistry::ReloadTextures()
	{
		ReloadModels();

		m_Blocks.clear();

		for (const auto& registration : m_Registrations)
			Register(registration.id, registration.variantIndex, registration.textures, registration.textureModel);

		for (const auto& overlay : m_RegistrationsOverlays)
			RegisterOverlay(overlay.id, overlay.variantIndex, overlay.textures);
	}

	const std::unordered_set<std::string>& BlockRegistry::GetAllTextureNames() const
	{
		return m_AllTextureNames;
	}

	const BlockTextures& BlockRegistry::Get(BlockId id) const
	{
		return Get(id, 0);
	}

	const BlockTextures& BlockRegistry::Get(BlockId id, uint8_t variantIndex) const
	{
		auto it = m_Blocks.find(id);
		if (it == m_Blocks.end() || it->second.empty())
		{
			throw std::runtime_error("BlockRegistry::Get: Block ID not found: " +
									 std::to_string(static_cast<uint16_t>(id)));
		}

		// Clamp to valid range — prevents crashes from stale variant indices
		const auto& variants = it->second;
		const size_t idx = std::min(static_cast<size_t>(variantIndex), variants.size() - 1);
		return variants[idx];
	}

	size_t BlockRegistry::GetVariantCount(BlockId id) const
	{
		auto it = m_Blocks.find(id);
		if (it == m_Blocks.end())
			return 0;
		return it->second.size();
	}

	void BlockRegistry::ReloadModels()
	{
		BlockModel::ClearCache();

		m_AllTextureNames.clear();
		m_Registrations.clear();
		m_RegistrationsOverlays.clear();

		for (uint16_t id = 0; id < static_cast<uint16_t>(BlockId::Count); id++)
		{
			BlockId blockId = static_cast<BlockId>(id);
			RegisterModel(blockId);
		}

		//// ---- Custom Blocks (not in Minecraft, or with special texture requirements) ----
		//RegisterModel(BlockId::SnowGrassBlock,
		//			  {TextureInfo{"block/snow.png", Tint::None},
		//			   TextureInfo{"block/dirt.png", Tint::None},
		//			   TextureInfo{"block/grass_block_snow.png", Tint::None},
		//			   TextureInfo{"block/grass_block_snow.png", Tint::None},
		//			   TextureInfo{"block/grass_block_snow.png", Tint::None},
		//			   TextureInfo{"block/grass_block_snow.png", Tint::None}},
		//			  Model::Block,
		//			  0);

		// ---- Reload Atlas Textures ----
		m_Atlas->ReloadTextures(m_AllTextureNames);
	}

} // namespace onion::voxel
