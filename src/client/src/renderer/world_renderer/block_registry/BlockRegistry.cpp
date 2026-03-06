#include "BlockRegistry.hpp"

namespace onion::voxel
{
	BlockRegistry::BlockRegistry(std::shared_ptr<TextureAtlas> atlas) : m_Atlas(atlas) {}

	void BlockRegistry::Register(BlockId id, std::array<std::string, 6> textures)
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
