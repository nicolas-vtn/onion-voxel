#pragma once

#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <glm/glm.hpp>

#include <renderer/texture_atlas/TextureAtlas.hpp>

#include <shared/world/block/Block.hpp>
#include <shared/world/block/BlockModel.hpp>
#include <shared/world/block/BlockstateRegistry.hpp>

namespace onion::voxel
{
	enum class Face : uint8_t
	{
		Top,
		Bottom,
		Front,
		Back,
		Left,
		Right,
		Count
	};

	enum class Tint : uint8_t
	{
		None,
		Grass,
		Water
	};

	struct TextureInfo
	{
		TextureAtlas::TextureID texture = UINT16_MAX;
		std::string name;
		Face face;
		Tint tintType = Tint::None;
		Transparency textureType = Transparency::Opaque;
		glm::vec3 from = {0, 0, 0};
		glm::vec3 to = {16, 16, 16};
		std::array<float, 4> uv = {0, 0, 16, 16}; // per-face UV override [u1,v1,u2,v2] in MC units (0-16)
		int uvRotation = 0;						  // per-face UV rotation in degrees (0, 90, 180, 270)
		BlockModel::ElementRotation elemRotation; // element-level 3D rotation (axis/angle/origin)
		bool shade = true;
	};

	struct BlockTextures
	{
		std::vector<TextureInfo> faces;
		std::vector<TextureInfo> overlay;
	};

	class BlockRenderRegistry
	{
		// ----- Constructor / Destructor -----
	  public:
		BlockRenderRegistry(std::shared_ptr<TextureAtlas> atlas);

		// ----- Public API -----
	  public:
		void Initialize();
		void ReloadTextures();
		const std::unordered_set<std::string>& GetAllTextureNames() const;

		// Get BlockTextures for the default (first) variant
		const BlockTextures& Get(BlockId id) const;

		// Get BlockTextures for a specific variant index
		const BlockTextures& Get(BlockId id, uint8_t variantIndex) const;

		// Returns the number of registered variants for a block
		size_t GetVariantCount(BlockId id) const;

		// ----- Private Methods -----
	  private:
		void ReloadModels();

		// Load and stage all variants for a block from its blockstate JSON
		void LoadBlockModel(BlockId id);
		// Load and stage a specific variant (by VariantModel) — used internally
		void LoadVariant(BlockId id, const VariantModel& variant, size_t variantIndex);
		// Stage a direct texture array (used by hand-crafted special cases)
		void StageTextures(BlockId id, const std::vector<TextureInfo>& textures, size_t variantIndex);

		void StageOverlayTextures(BlockId id, const std::vector<TextureInfo>& textures, size_t variantIndex);

		// ----- Atlas Commit (resolves texture IDs from atlas) -----
	  private:
		void CommitTextures(BlockId id, uint8_t variantIndex, const std::vector<TextureInfo>& textures);
		void CommitOverlayTextures(BlockId id, uint8_t variantIndex, const std::vector<TextureInfo>& textures);

		// ----- Private Members -----
	  private:
		struct StagedTextures
		{
			BlockId id;
			uint8_t variantIndex; // which slot within m_Blocks[id]
			std::vector<TextureInfo> textures;
		};

		struct StagedOverlayTextures
		{
			BlockId id;
			uint8_t variantIndex; // which slot within m_Blocks[id]
			std::vector<TextureInfo> textures;
		};

		std::vector<StagedTextures> m_StagedTextures;
		std::vector<StagedOverlayTextures> m_StagedOverlayTextures;

		// Each block can have multiple variants; index 0 is always the default
		std::unordered_map<BlockId, std::vector<BlockTextures>> m_Blocks;
		std::shared_ptr<TextureAtlas> m_Atlas;
		std::unordered_set<std::string> m_AllTextureNames;
	};
} // namespace onion::voxel
