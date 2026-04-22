#pragma once

#include <array>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <glm/glm.hpp>

#include <renderer/texture_atlas/TextureAtlas.hpp>

#include <shared/world/block/Block.hpp>

#include "BlockStateJson.hpp"

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
		Foliage,
		Water
	};

	enum class Model : uint8_t
	{
		Block,
		Cross
	};

	struct FaceTexture
	{
		TextureAtlas::TextureID texture = UINT16_MAX;
		Tint tintType = Tint::None;
		Transparency textureType = Transparency::Opaque;
		glm::u8vec3 from = {0, 0, 0};				// element start corner in MC units (0-16)
		glm::u8vec3 to = {16, 16, 16};				// element end corner in MC units (0-16)
		std::array<uint8_t, 4> uv = {0, 0, 16, 16}; // per-face UV override [u1,v1,u2,v2] in MC units (0-16)
	};

	struct TextureInfo
	{
		std::string name;
		Tint tintType = Tint::None;
		glm::u8vec3 from = {0, 0, 0};
		glm::u8vec3 to = {16, 16, 16};
		std::array<uint8_t, 4> uv = {0, 0, 16, 16}; // per-face UV override [u1,v1,u2,v2] in MC units (0-16)
	};

	struct BlockTextures
	{
		std::array<FaceTexture, (size_t) Face::Count> faces;
		std::array<FaceTexture, (size_t) Face::Count> overlay;
		Model textureModel = Model::Block;
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

		// Returns a map of BlockId -> variant count for all registered blocks.
		// Intended to be passed to WorldGenerator::SetVariantCounts() after initialization.
		std::unordered_map<BlockId, uint8_t> GetAllVariantCounts() const;

		// Given a set of blockstate properties (e.g. {"axis"->"x"}), return the
		// matching variant index for this block. Returns 0 if no match is found.
		uint8_t ResolveVariantIndex(BlockId id, const std::map<std::string, std::string>& properties) const;

		// ----- Private Methods -----
	  private:
		void ReloadModels();

		// Register all variants parsed from a blockstate JSON file
		void RegisterModel(BlockId id, const std::string& blockstate);
		// Register a specific variant (by VariantModel) — used internally
		void RegisterVariant(BlockId id, const VariantModel& variant);
		// Register a direct texture array (used by hand-crafted special cases)
		void RegisterModel(BlockId id, const std::array<TextureInfo, 6>& textures, Model textureModel);

		void PreSetOverlay(BlockId id, Face face, const TextureInfo& texture);

		// ----- Real Registrations -----
	  private:
		void Register(BlockId id,
					  uint8_t variantIndex,
					  const std::array<TextureInfo, 6>& textures,
					  Model textureModel = Model::Block);
		void SetOverlay(BlockId id, Face face, const TextureInfo& texture);

		// ----- Private Members -----
	  private:
		struct PreRegistration
		{
			BlockId id;
			uint8_t variantIndex; // which slot within m_Blocks[id]
			std::array<TextureInfo, 6> textures;
			Model textureModel;
		};

		struct PreOverlayRegistration
		{
			BlockId id;
			Face face;
			TextureInfo texture;
		};

		// Tracks how many variants have been pre-registered per block
		std::unordered_map<BlockId, uint8_t> m_VariantCounters;

		// Stores parsed variant conditions so ResolveVariantIndex can match them
		std::unordered_map<BlockId, std::vector<BlockStateVariant>> m_VariantConditions;

		std::vector<PreRegistration> m_Registrations;
		std::vector<PreOverlayRegistration> m_RegistrationsOverlays;

		// Each block can have multiple variants; index 0 is always the default
		std::unordered_map<BlockId, std::vector<BlockTextures>> m_Blocks;
		std::shared_ptr<TextureAtlas> m_Atlas;
		std::unordered_set<std::string> m_AllTextureNames;
	};
} // namespace onion::voxel
