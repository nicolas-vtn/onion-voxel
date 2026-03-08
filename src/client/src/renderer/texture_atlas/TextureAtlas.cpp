#include "TextureAtlas.hpp"

#include <cmath>
#include <stb_image.h>
#include <stdexcept>

namespace onion::voxel
{

	TextureAtlas::TextureAtlas(const std::filesystem::path& directory) : m_Directory(directory)
	{
		ScanTextures();
		BuildAtlas();
	}

	void TextureAtlas::Bind() const
	{
		m_Texture.Bind();
	}

	TextureAtlas::TextureID TextureAtlas::GetTextureID(const std::string& name) const
	{
		auto it = m_NameToID.find(name);

		if (it == m_NameToID.end())
			throw std::runtime_error("Texture not found: " + name);

		return it->second;
	}

	const TextureAtlas::AtlasEntry& TextureAtlas::GetAtlasEntry(TextureID id) const
	{
		if (id >= m_Entries.size())
			throw std::out_of_range("TextureID out of range");

		return m_Entries[id];
	}

	void TextureAtlas::ScanTextures()
	{
		for (auto& entry : std::filesystem::directory_iterator(m_Directory))
		{
			if (entry.path().extension() == ".png")
			{
				m_TextureFiles.push_back(entry.path());
			}
		}
	}

	void TextureAtlas::BuildAtlas()
	{
		size_t count = m_TextureFiles.size();

		int grid = (int) std::ceil(std::sqrt((float) count));

		// Loads a texture to check the texture size, assumes all textures are the same size
		{
			int w, h, channels;

			stbi_set_flip_vertically_on_load(true);
			unsigned char* pixels = stbi_load((m_Directory / "stone.png").string().c_str(), &w, &h, &channels, 4);

			if (!pixels)
				throw std::runtime_error("Failed loading texture");

			stbi_image_free(pixels);

			m_TextureSize = w;
		}

		m_AtlasSize = grid * m_TextureSize;

		std::vector<unsigned char> atlasPixels(m_AtlasSize * m_AtlasSize * 4, 0);

		m_Entries.resize(count);

		for (size_t i = 0; i < count; i++)
		{
			int x = (i % grid);
			int y = (i / grid);

			int atlasX = x * m_TextureSize;
			int atlasY = y * m_TextureSize;

			int w, h, channels;

			stbi_set_flip_vertically_on_load(true);
			unsigned char* pixels = stbi_load(m_TextureFiles[i].string().c_str(), &w, &h, &channels, 4);

			if (!pixels)
				throw std::runtime_error("Failed loading texture");

			for (int row = 0; row < m_TextureSize; row++)
			{
				memcpy(&atlasPixels[((atlasY + row) * m_AtlasSize + atlasX) * 4],
					   &pixels[row * m_TextureSize * 4],
					   m_TextureSize * 4);
			}

			stbi_image_free(pixels);

			TextureID id = (TextureID) i;

			std::string name = m_TextureFiles[i].filename().string();

			m_NameToID[name] = id;

			float u0 = (float) atlasX / (float) m_AtlasSize;
			float v0 = (float) atlasY / (float) m_AtlasSize;

			float u1 = (float) (atlasX + m_TextureSize) / (float) m_AtlasSize;
			float v1 = (float) (atlasY + m_TextureSize) / (float) m_AtlasSize;

			m_Entries[id] = {{u0, v0}, {u1, v1}};
		}

		m_Texture = Texture(atlasPixels, m_AtlasSize, m_AtlasSize, 4);
	}

} // namespace onion::voxel
