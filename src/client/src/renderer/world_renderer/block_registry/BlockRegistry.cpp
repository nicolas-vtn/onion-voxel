#include "BlockRegistry.hpp"

namespace onion::voxel
{
	BlockRegistry::BlockRegistry(std::shared_ptr<TextureAtlas> atlas) : m_Atlas(atlas)
	{
		Register(BlockId::Stone, "stone.png");
		Register(BlockId::Dirt, "dirt.png");

		std::array<std::string, 6> grassTextures = {"grass_block_top.png",
													"dirt.png",
													"grass_block_side.png",
													"grass_block_side.png",
													"grass_block_side.png",
													"grass_block_side.png"};
		Register(BlockId::Grass, grassTextures);
		SetOverlay(BlockId::Grass, BlockFace::Front, "grass_block_side_overlay.png");
		SetOverlay(BlockId::Grass, BlockFace::Back, "grass_block_side_overlay.png");
		SetOverlay(BlockId::Grass, BlockFace::Left, "grass_block_side_overlay.png");
		SetOverlay(BlockId::Grass, BlockFace::Right, "grass_block_side_overlay.png");
	}

	void BlockRegistry::Register(BlockId id, const std::array<std::string, 6>& textures)
	{
		BlockTextures tex;

		for (size_t i = 0; i < 6; ++i)
		{
			tex.faces[i].texture = m_Atlas->GetTextureID(textures[i]);
		}

		m_Blocks[id] = tex;
	}

	void BlockRegistry::Register(BlockId id, const std::string& texture)
	{
		std::array<std::string, 6> textures;
		textures.fill(texture);
		Register(id, textures);
	}

	void BlockRegistry::SetOverlay(BlockId id, BlockFace face, const std::string& texture)
	{
		auto it = m_Blocks.find(id);
		if (it == m_Blocks.end())
		{
			throw std::runtime_error("BlockRegistry::SetOverlay: Block ID not found: " +
									 std::to_string(static_cast<uint8_t>(id)));
		}
		it->second.overlay[static_cast<size_t>(face)].texture = m_Atlas->GetTextureID(texture);
	}

	const BlockTextures& BlockRegistry::Get(BlockId id) const
	{
		auto it = m_Blocks.find(id);
		if (it == m_Blocks.end())
		{
			throw std::runtime_error("BlockRegistry::Get: Block ID not found: " +
									 std::to_string(static_cast<uint8_t>(id)));
		}

		return it->second;
	}
} // namespace onion::voxel
