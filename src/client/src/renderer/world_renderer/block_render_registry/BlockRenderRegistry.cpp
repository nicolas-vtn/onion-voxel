#include "BlockRenderRegistry.hpp"

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
			return Face::Up;
		if (name == "down")
			return Face::Down;
		if (name == "north")
			return Face::North;
		if (name == "south")
			return Face::South;
		if (name == "west")
			return Face::West;
		if (name == "east")
			return Face::East;
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
// onion::voxel::BlockRenderRegistry implementation
// ---------------------------------------------------------------------------
namespace onion::voxel
{
	BlockRenderRegistry::BlockRenderRegistry(std::shared_ptr<TextureAtlas> atlas) : m_Atlas(atlas) {}

	// ---- Stage a direct texture array (used by special-case blocks) ----
	void BlockRenderRegistry::StageTextures(BlockId id, const std::vector<TextureInfo>& textures, size_t variantIndex)
	{
		for (const auto& texture : textures)
			if (!texture.name.empty())
				m_AllTextureNames.insert(texture.name);

		StagedTextures staged;
		staged.id = id;
		staged.variantIndex = static_cast<uint8_t>(variantIndex);
		staged.textures = textures;
		m_StagedTextures.push_back(std::move(staged));
	}

	void
	BlockRenderRegistry::StageOverlayTextures(BlockId id, const std::vector<TextureInfo>& textures, size_t variantIndex)
	{
		for (const auto& texture : textures)
			if (!texture.name.empty())
				m_AllTextureNames.insert(texture.name);

		StagedOverlayTextures staged;
		staged.id = id;
		staged.variantIndex = static_cast<uint8_t>(variantIndex);
		staged.textures = textures;

		m_StagedOverlayTextures.push_back(std::move(staged));
	}

	// ---- Load one VariantModel for a block ----
	void BlockRenderRegistry::LoadVariant(BlockId id, const VariantModel& variant, size_t variantIndex)
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
								 static_cast<int>(face.Rotation.value_or(0.f)),
								 elem.Rotation,
								 elem.Shade};

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
			StageTextures(id, baseTextures, variantIndex);

		if (overlayInitialized)
			StageOverlayTextures(id, overlayTextures, variantIndex);
	}

	// ---- Load all variants parsed from a blockstate JSON ----
	void BlockRenderRegistry::LoadBlockModel(BlockId id)
	{
		const auto& blockstates = BlockstateRegistry::Get();

		const auto it = blockstates.find(id);
		if (it == blockstates.end())
		{
			std::cout << "Warning: No blockstate found for block ID " << static_cast<uint16_t>(id) << " ("
					  << BlockIds::GetName(id) << ")." << std::endl;

			VariantModel defaultVariant;
			LoadVariant(id, defaultVariant, 0);

			return;
		}

		const auto& variants = it->second;

		for (int i = 0; i < variants.size(); i++)
			LoadVariant(id, variants[i], i);
	}

	// ---- Commit: resolve TextureIDs from atlas ----
	void BlockRenderRegistry::CommitTextures(BlockId id, uint8_t variantIndex, const std::vector<TextureInfo>& textures)
	{
		BlockTextures tex;

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

	void BlockRenderRegistry::CommitOverlayTextures(BlockId id,
													uint8_t variantIndex,
													const std::vector<TextureInfo>& textures)
	{
		auto it = m_Blocks.find(id);
		if (it == m_Blocks.end() || it->second.empty())
		{
			throw std::runtime_error("BlockRenderRegistry::CommitOverlayTextures: Block ID not found: " +
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

	void BlockRenderRegistry::Initialize()
	{
		ReloadTextures();
	}

	void BlockRenderRegistry::ReloadTextures()
	{
		ReloadModels();

		m_Blocks.clear();

		for (const auto& staged : m_StagedTextures)
			CommitTextures(staged.id, staged.variantIndex, staged.textures);

		for (const auto& staged : m_StagedOverlayTextures)
			CommitOverlayTextures(staged.id, staged.variantIndex, staged.textures);

		// Build the per-variant full-block lookup now that all variants are committed
		// and the registry is fully populated.
		BlockState::BuildFullBlockLookup();
	}

	const std::unordered_set<std::string>& BlockRenderRegistry::GetAllTextureNames() const
	{
		return m_AllTextureNames;
	}

	const BlockTextures& BlockRenderRegistry::Get(BlockId id) const
	{
		return Get(id, 0);
	}

	const BlockTextures& BlockRenderRegistry::Get(BlockId id, uint8_t variantIndex) const
	{
		auto it = m_Blocks.find(id);
		if (it == m_Blocks.end() || it->second.empty())
		{
			throw std::runtime_error("BlockRenderRegistry::Get: Block ID not found: " +
									 std::to_string(static_cast<uint16_t>(id)));
		}

		const auto& variants = it->second;
		return variants[variantIndex];
	}

	size_t BlockRenderRegistry::GetVariantCount(BlockId id) const
	{
		auto it = m_Blocks.find(id);
		if (it == m_Blocks.end())
			return 0;
		return it->second.size();
	}

	void BlockRenderRegistry::ReloadModels()
	{
		BlockModel::ClearCache();

		m_AllTextureNames.clear();
		m_StagedTextures.clear();
		m_StagedOverlayTextures.clear();

		for (uint16_t id = 0; id < static_cast<uint16_t>(BlockId::Count); id++)
		{
			BlockId blockId = static_cast<BlockId>(id);
			LoadBlockModel(blockId);
		}

		// ---- Reload Atlas Textures ----
		m_Atlas->ReloadTextures(m_AllTextureNames);
	}

} // namespace onion::voxel
