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

	class BlockRegistry
	{
		// ----- Constructor / Destructor -----
	  public:
		BlockRegistry(std::shared_ptr<TextureAtlas> atlas);

		// ----- Public API -----
	  public:
		void Initialize();
		void ReloadTextures();
		const std::unordered_set<std::string>& GetAllTextureNames() const;

		// Get BlockTextures for the default (first) variant
		const BlockTextures& Get(BlockId id) const;

		// Get BlockTextures for a specific variant index (clamped to valid range)
		const BlockTextures& Get(BlockId id, uint8_t variantIndex) const;

		// Returns the number of registered variants for a block (at least 1 if the block exists)
		size_t GetVariantCount(BlockId id) const;

		// ----- Private Methods -----
	  private:
		void ReloadModels();

		// Register all variants for a block
		void RegisterModel(BlockId id);
		// Register a specific variant (by VariantModel) — used internally
		void RegisterVariant(BlockId id, const VariantModel& variant, size_t variantIndex);
		// Register a direct texture array (used by hand-crafted special cases)
		void RegisterModel(BlockId id, const std::vector<TextureInfo>& textures, size_t variantIndex);

		void RegisterModelOverlay(BlockId id, const std::vector<TextureInfo>& textures, size_t variantIndex);

		// ----- Real Registrations -----
	  private:
		void Register(BlockId id, uint8_t variantIndex, const std::vector<TextureInfo>& textures);
		void RegisterOverlay(BlockId id, uint8_t variantIndex, const std::vector<TextureInfo>& textures);

		// ----- Private Members -----
	  private:
		struct PreRegistration
		{
			BlockId id;
			uint8_t variantIndex; // which slot within m_Blocks[id]
			std::vector<TextureInfo> textures;
		};

		struct PreOverlayRegistration
		{
			BlockId id;
			uint8_t variantIndex; // which slot within m_Blocks[id]
			std::vector<TextureInfo> textures;
		};

		std::vector<PreRegistration> m_Registrations;
		std::vector<PreOverlayRegistration> m_RegistrationsOverlays;

		// Each block can have multiple variants; index 0 is always the default
		std::unordered_map<BlockId, std::vector<BlockTextures>> m_Blocks;
		std::shared_ptr<TextureAtlas> m_Atlas;
		std::unordered_set<std::string> m_AllTextureNames;
	};
} // namespace onion::voxel
