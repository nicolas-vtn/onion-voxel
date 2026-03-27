#include "texture_tile_mapper.hpp"

namespace onion::voxel
{

	TextureTileMapper::TextureTileMapper() {}

	TextureTileMapper::~TextureTileMapper() {}

	void TextureTileMapper::SetTextureDimensions(int width, int height)
	{
		m_TextureWidth = width;
		m_TextureHeight = height;
	}

	void TextureTileMapper::AddTextureTile(int textureId, glm::ivec2 bottomLeft_px, glm::ivec2 topRight_px)
	{

		if (m_TextureWidth == -1 || m_TextureHeight == -1)
		{
			std::cout << "[TextureTileMapper] [Error] Texture dimensions not set before adding texture tiles.\n";
			return;
		}

		TextureTile tile(bottomLeft_px, topRight_px, m_TextureWidth, m_TextureHeight, m_FlipY);

		m_TextureTiles[textureId] = tile;
	}

	void TextureTileMapper::AddTextureTile(
		int textureId, int bottomLeft_X_px, int bottomLeft_Y_px, int topRight_X_px, int topRight_Y_px)
	{
		AddTextureTile(
			textureId, glm::ivec2(bottomLeft_X_px, bottomLeft_Y_px), glm::ivec2(topRight_X_px, topRight_Y_px));
	}

	void TextureTileMapper::SetFlipY(bool flip)
	{
		m_FlipY = flip;
	}

	const TextureTile& TextureTileMapper::GetTextureTile(int textureId) const
	{
		auto it = m_TextureTiles.find(textureId);
		if (it != m_TextureTiles.end())
		{
			return it->second;
		}

		std::cout << "[TextureTileMapper] [Missing Key] Key '" << textureId << "' not found!\n";
		static TextureTile dummy{}; // Safe fallback
		return dummy;
	}
} // namespace onion::voxel
