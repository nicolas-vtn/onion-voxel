#pragma once

#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <glm/glm.hpp>

#include <renderer/texture_atlas/TextureAtlas.hpp>

#include <shared/world/block/Block.hpp>

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
		TextureAtlas::TextureID texture = 0;
		Tint tintType = Tint::None;
		Transparency textureType = Transparency::Opaque;
		glm::u8vec3 from = {0, 0, 0};        // element start corner in MC units (0-16)
		glm::u8vec3 to   = {16, 16, 16};     // element end corner in MC units (0-16)
		std::array<uint8_t, 4> uv = {0, 0, 16, 16}; // per-face UV override [u1,v1,u2,v2] in MC units (0-16)
	};

	struct TextureInfo
	{
		std::string name;
		Tint tintType = Tint::None;
		glm::u8vec3 from = {0, 0, 0};
		glm::u8vec3 to   = {16, 16, 16};
		std::array<uint8_t, 4> uv = {0, 0, 16, 16}; // per-face UV override [u1,v1,u2,v2] in MC units (0-16)
	};

	struct BlockTextures
	{
		std::array<FaceTexture, (size_t) Face::Count> faces;
		std::array<FaceTexture, (size_t) Face::Count> overlay;
		BlockState::RotationType rotationType = BlockState::RotationType::None;
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
		const BlockTextures& Get(BlockId id) const;

		// ----- Private Methods -----
	  private:
		void ReloadModels();

		void RegisterModel(BlockId id, const std::filesystem::path& model);
		void RegisterModel(BlockId id, const std::array<TextureInfo, 6>& textures, Model textureModel);

		void PreSetOverlay(BlockId id, Face face, const TextureInfo& texture);

		// ----- Real Registrations -----
	  private:
		void Register(BlockId id, const std::array<TextureInfo, 6>& textures, Model textureModel = Model::Block);
		void SetOverlay(BlockId id, Face face, const TextureInfo& texture);

		// ----- Private Members -----
	  private:
		struct PreRegistration
		{
			BlockId id;
			std::array<TextureInfo, 6> textures;
			Model textureModel;
		};

		struct PreOverlayRegistration
		{
			BlockId id;
			Face face;
			TextureInfo texture;
		};

		std::vector<PreRegistration> m_Registrations;
		std::vector<PreOverlayRegistration> m_RegistrationsOverlays;

		std::unordered_map<BlockId, BlockTextures> m_Blocks;
		std::shared_ptr<TextureAtlas> m_Atlas;
		std::unordered_set<std::string> m_AllTextureNames;
	};
} // namespace onion::voxel
