#include "texture.hpp"

#include <iostream>

#include <glad/glad.h>

namespace
{
	void FreePixels(unsigned char* ptr)
	{
		if (ptr)
		{
			stbi_image_free(ptr); // or delete[] ptr;
		}
	}
} // namespace

namespace onion::voxel
{
	Texture::Texture(const std::string& filePath)
	{
		if (!LoadFromFile(filePath))
		{
			std::cout << "[TEXTURE] [ERROR] : Failed to load texture from file: " << filePath << std::endl;
		}
	}

	Texture::~Texture()
	{
		if (m_TextureID != 0)
		{
			std::cout << "[TEXTURE] [ERROR] : Texture '" << m_FilePath
					  << "' not deleted before destruction. There is a memory leak." << std::endl;
		}
	}

	bool Texture::LoadFromFile(const std::string& filePath)
	{
		m_FilePath = filePath;
		int width, height, nrChannels;
		stbi_set_flip_vertically_on_load(false); // For OpenGL coordinate system
		unsigned char* data = stbi_load(m_FilePath.c_str(), &width, &height, &nrChannels, 0);
		if (!data)
		{
			// handle error
			std::cout << "[TEXTURE] [ERROR] : Failed to load texture: " << m_FilePath << std::endl;
			return false;
		}

		// Store texture info
		m_Width = width;
		m_Height = height;
		m_NrChannels = nrChannels;

		// Saves the raw data, it will be freed after uploading to GPU
		m_Data = data;

		return true;
	}

	void Texture::UploadToGPU() const
	{
		if (!m_Data)
		{
			std::cout << "[TEXTURE] [ERROR] : No data to upload for texture: " << m_FilePath << std::endl;
			return;
		}

		glGenTextures(1, &m_TextureID);
		glBindTexture(GL_TEXTURE_2D, m_TextureID);

		// Safe default for any channel count
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		GLenum srcFormat = GL_RGB;
		GLenum internalFormat = GL_RGB;

		if (m_NrChannels == 1)
		{
			srcFormat = GL_RED;
			internalFormat = GL_R8; // 8-bit single channel

			// Optional but recommended: show as grayscale when sampling as vec4
			GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_ONE};
			glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
		}
		else if (m_NrChannels == 3)
		{
			srcFormat = GL_RGB;
			internalFormat = GL_RGB8;
		}
		else if (m_NrChannels == 4)
		{
			srcFormat = GL_RGBA;
			internalFormat = GL_RGBA8;
		}
		else
		{
			std::cout << "[TEXTURE] [ERROR] : Unsupported channel count (" << m_NrChannels
					  << ") for texture: " << m_FilePath << std::endl;
			glBindTexture(GL_TEXTURE_2D, 0);
			return;
		}

		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, srcFormat, GL_UNSIGNED_BYTE, m_Data);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		stbi_image_free(m_Data);
		m_Data = nullptr;

		m_HasBeenUploadedToGPU = true;
	}

	void Texture::Bind() const
	{

		if (!m_HasBeenUploadedToGPU)
		{
			UploadToGPU();
		}

		glActiveTexture(GL_TEXTURE0);			   // Activate texture slot 0
		glBindTexture(GL_TEXTURE_2D, m_TextureID); // Bind our atlas to slot 0
	}

	void Texture::Unbind() const
	{
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void Texture::Delete()
	{
		glDeleteTextures(1, &m_TextureID);
		m_TextureID = 0;
	}

	using PixelDeleter = void (*)(unsigned char*);
	std::unique_ptr<unsigned char[], PixelDeleter> Texture::GetData() const
	{
		int width, height, nrChannels;
		stbi_set_flip_vertically_on_load(false); // For OpenGL coordinate system
		unsigned char* data = stbi_load(m_FilePath.c_str(), &width, &height, &nrChannels, 0);
		if (!data)
		{
			// handle error
			std::cout << "[TEXTURE] [ERROR] : Failed to load texture: " << m_FilePath << std::endl;
			return {nullptr, FreePixels};
		}

		return std::unique_ptr<unsigned char[], PixelDeleter>(data, FreePixels);
	}

	unsigned int Texture::GetTextureID() const
	{
		return m_TextureID;
	}

	bool Texture::HasBeenLoaded() const
	{
		return m_TextureID != 0;
	}

	int Texture::GetWidth() const
	{
		return m_Width;
	}
	int Texture::GetHeight() const
	{
		return m_Height;
	}
	int Texture::GetNrChannels() const
	{
		return m_NrChannels;
	}
}; // namespace onion::voxel
