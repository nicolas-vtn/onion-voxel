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

				TextureInfo info{UINT16_MAX,
								 resolved,
								 f,
								 tint,
								 Transparency::Opaque,
								 elem.From,
								 elem.To,
								 face.UV,
								 static_cast<int>(face.Rotation.value_or(0.f))};

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

			TextureInfo resolvedInfo = textureInfo;
			resolvedInfo.texture = m_Atlas->GetTextureID(textureName);

			const Transparency transparency = m_Atlas->GetTextureTransparency(textureName);
			resolvedInfo.textureType = transparency;

			// Update the transparency type of the block
			if (transparency == Transparency::Transparent || transparency == Transparency::Cutout)
			{
				BlockState::SetTransparency(id, true);
			}

			tex.faces.push_back(resolvedInfo);

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

		for (const auto& textureInfo : textures)
		{
			const std::string& textureName = textureInfo.name;

			if (textureName.empty())
				continue;

			TextureInfo resolvedInfo = textureInfo;
			resolvedInfo.texture = m_Atlas->GetTextureID(textureName);
			resolvedInfo.textureType = m_Atlas->GetTextureTransparency(textureName);

			blockTex.overlay.push_back(resolvedInfo);
			m_AllTextureNames.insert(textureName);
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
