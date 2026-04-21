#include "TextureAtlas.hpp"

#include <cmath>
#include <stb_image.h>
#include <stdexcept>

namespace onion::voxel
{

	TextureAtlas::TextureAtlas() {}

	void TextureAtlas::Bind() const
	{
		m_Texture.Bind();
	}

	void TextureAtlas::Initialize(const std::unordered_set<std::string>& textureNames)
	{
		BuildAtlas(textureNames);
	}

	void TextureAtlas::ReloadTextures(const std::unordered_set<std::string>& textureNames)
	{
		BuildAtlas(textureNames);
	}

	void TextureAtlas::Unload()
	{
		m_Texture.Delete();
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

	Transparency TextureAtlas::GetTextureTransparency(const std::string& name) const
	{
		auto it = m_NameToTransparency.find(name);

		if (it == m_NameToTransparency.end())
			throw std::runtime_error("Texture not found: " + name);

		return it->second;
	}

	void TextureAtlas::BuildAtlas(const std::unordered_set<std::string>& textureNames)
	{
		m_Texture.Delete();

		size_t count = textureNames.size();

		int grid = (int) std::ceil(std::sqrt((float) count));

		// Loads a texture to check the texture size, assumes all textures are the same size
		{
			int w, h, channels;

			stbi_set_flip_vertically_on_load(true);

			std::filesystem::path texturePath = s_BlockDirectory / *textureNames.begin();
			std::vector<unsigned char> pixels = EngineContext::Get().Assets->GetResourcePackFileBinary(texturePath);

			unsigned char* data = stbi_load_from_memory(pixels.data(), (int) pixels.size(), &w, &h, &channels, 4);

			if (!data)
				throw std::runtime_error("Failed loading texture");

			stbi_image_free(data);

			m_TextureSize = w;
		}

		m_AtlasSize = grid * m_TextureSize;

		std::vector<unsigned char> atlasPixels(m_AtlasSize * m_AtlasSize * 4, 0);

		m_Entries.resize(count);

		for (size_t i = 0; i < count; i++)
		{
			int x = i % grid;
			int y = static_cast<int>(i / grid);

			int atlasX = x * m_TextureSize;
			int atlasY = y * m_TextureSize;

			int w, h, channels;

			stbi_set_flip_vertically_on_load(true);

			auto it = textureNames.begin();
			std::advance(it, i);
			std::filesystem::path texturePath = s_BlockDirectory / *it;
			std::vector<unsigned char> data = EngineContext::Get().Assets->GetResourcePackFileBinary(texturePath);

			unsigned char* pixels = stbi_load_from_memory(data.data(), (int) data.size(), &w, &h, &channels, 4);

			if (!pixels)
				throw std::runtime_error("Failed loading texture");

			for (int row = 0; row < m_TextureSize; row++)
			{
				memcpy(&atlasPixels[((atlasY + row) * m_AtlasSize + atlasX) * 4],
					   &pixels[row * m_TextureSize * 4],
					   m_TextureSize * 4);
			}

			TextureID id = (TextureID) i;

			std::string name = *it;

			m_NameToID[name] = id;
			m_NameToTransparency[name] = GetTextureTransparency(pixels, w, h, 4);

			float u0 = (float) atlasX / (float) m_AtlasSize;
			float v0 = (float) atlasY / (float) m_AtlasSize;

			float u1 = (float) (atlasX + m_TextureSize) / (float) m_AtlasSize;
			float v1 = (float) (atlasY + m_TextureSize) / (float) m_AtlasSize;

			m_Entries[id] = {{u0, v0}, {u1, v1}};

			stbi_image_free(pixels);
		}

		m_Texture = Texture("TextureAtlas", atlasPixels, m_AtlasSize, m_AtlasSize, 4);
	}

	Transparency TextureAtlas::GetTextureTransparency(const unsigned char* pixels, int width, int height, int channels)
	{
		if (pixels == nullptr || width <= 0 || height <= 0)
			return Transparency::Opaque;

		// Supported common layouts:
		// 1 = Gray
		// 2 = Gray + Alpha
		// 3 = RGB
		// 4 = RGBA

		const bool hasAlpha = (channels == 2 || channels == 4);
		if (!hasAlpha)
			return Transparency::Opaque;

		const int alphaIndex = channels - 1;

		bool hasZeroAlpha = false;

		for (size_t i = 0; i + alphaIndex < static_cast<size_t>(width * height * channels); i += channels)
		{
			const unsigned char alpha = pixels[i + alphaIndex];

			if (alpha > 0 && alpha < 255)
				return Transparency::Transparent;

			if (alpha == 0)
				hasZeroAlpha = true;
		}

		return hasZeroAlpha ? Transparency::Cutout : Transparency::Opaque;
	}

} // namespace onion::voxel
