#pragma once

#include <array>
#include <memory>
#include <string>
#include <unordered_map>

#include "../../texture_atlas/TextureAtlas.hpp"
#include <shared/world/block/Block.hpp>

namespace onion::voxel
{
	enum class BlockFace : uint8_t
	{
		Top,
		Bottom,
		Front,
		Back,
		Left,
		Right,
		Count
	};

	enum class TintType : uint8_t
	{
		None,
		Grass,
		Foliage,
		Water
	};

	enum class TextureType : uint8_t
	{
		Opaque,
		Cutout,
		Transparent
	};

	struct FaceTexture
	{
		TextureAtlas::TextureID texture = 0;
		TintType tintType = TintType::None;
		TextureType textureType = TextureType::Opaque;
	};

	struct TextureInfo
	{
		std::string name;
		TintType tintType = TintType::None;
		TextureType textureType = TextureType::Opaque;
	};

	struct BlockTextures
	{
		std::array<FaceTexture, (size_t) BlockFace::Count> faces;
		std::array<FaceTexture, (size_t) BlockFace::Count> overlay;
		Block::RotationType rotationType = Block::RotationType::None;
	};

	class BlockRegistry
	{
		// ----- Constructor / Destructor -----
	  public:
		BlockRegistry(std::shared_ptr<TextureAtlas> atlas);

		// ----- Public API -----
	  public:
		void Register(BlockId id, const std::array<TextureInfo, 6>& textures);
		void Register(BlockId id, const TextureInfo& texture);
		void Register(BlockId id, const std::string& texture);

		void SetOverlay(BlockId id, BlockFace face, const TextureInfo& texture);

		const BlockTextures& Get(BlockId id) const;

		// ----- Private Members -----
	  private:
		std::unordered_map<BlockId, BlockTextures> m_Blocks;
		std::shared_ptr<TextureAtlas> m_Atlas;
	};
} // namespace onion::voxel
