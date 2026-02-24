#include "texture.hpp"

#include <iostream>

#include <glad/glad.h>

namespace
{
	void FreePixels(unsigned char* ptr)
	{
		if (ptr)
		{
			stbi_image_free(ptr);
		}
	}
} // namespace

namespace onion::voxel
{
	Texture::Texture() {}

	Texture::Texture(const std::filesystem::path& filePath) : m_FilePath(filePath)
	{
		if (!std::filesystem::exists(filePath))
		{
			std::cerr << "[TEXTURE] [ERROR] : Texture file does not exist: " << filePath << std::endl;
			throw std::runtime_error("Texture file does not exist: " + filePath.string());
		}

		if (!LoadFromFile(filePath))
		{
			std::cerr << "[TEXTURE] [ERROR] : Failed to load texture from file: " << filePath << std::endl;
			throw std::runtime_error("Failed to load texture from file: " + filePath.string());
		}
	}

	Texture::~Texture()
	{
		if (m_Data)
		{
			stbi_image_free(m_Data);
			m_Data = nullptr;
		}

		if (m_TextureID != 0)
		{
			std::cerr << "[TEXTURE] [ERROR] : Texture '" << m_FilePath
					  << "' not deleted before destruction. There is a memory leak." << std::endl;
		}
	}

	bool Texture::LoadFromFile(const std::filesystem::path& filePath)
	{
		m_FilePath = filePath;
		int width, height, nrChannels;
		stbi_set_flip_vertically_on_load(false); // For OpenGL coordinate system
		unsigned char* data = stbi_load(m_FilePath.string().c_str(), &width, &height, &nrChannels, 0);

		if (!data)
		{
			const char* reason = stbi_failure_reason();
			std::string error = reason ? reason : "Unknown error";
			std::cerr << "[TEXTURE] [ERROR] : Failed to load texture: " << m_FilePath << " Details: " << error
					  << std::endl;

			return false;
		}

		// Store texture info
		m_Width = width;
		m_Height = height;
		m_NbrChannels = nrChannels;

		// Saves the raw data, it will be freed after uploading to GPU
		m_Data = data;

		return true;
	}

	void Texture::UploadToGPU() const
	{
		if (!m_Data)
		{
			std::cerr << "[TEXTURE] [ERROR] : No data to upload for texture: " << m_FilePath << std::endl;
			return;
		}

		if (m_TextureID != 0)
		{
			std::cerr << "[TEXTURE] [WARNING] : Texture already has an OpenGL ID. Overwriting existing texture: "
					  << m_FilePath << std::endl;
			glDeleteTextures(1, &m_TextureID);
		}

		glGenTextures(1, &m_TextureID);
		glBindTexture(GL_TEXTURE_2D, m_TextureID);

		// Safe default for any channel count
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		GLenum srcFormat = GL_RGB;
		GLenum internalFormat = GL_RGB;

		if (m_NbrChannels == 1)
		{
			srcFormat = GL_RED;
			internalFormat = GL_R8; // 8-bit single channel

			// Show as grayscale when sampling as vec4
			GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_ONE};
			glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
		}
		else if (m_NbrChannels == 3)
		{
			srcFormat = GL_RGB;
			internalFormat = GL_RGB8;
		}
		else if (m_NbrChannels == 4)
		{
			srcFormat = GL_RGBA;
			internalFormat = GL_RGBA8;
		}
		else
		{
			std::cerr << "[TEXTURE] [ERROR] : Unsupported channel count (" << m_NbrChannels
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
		if (!std::filesystem::exists(m_FilePath))
		{
			std::cerr << "[TEXTURE] [ERROR] : Texture file does not exist: " << m_FilePath << std::endl;
			return {nullptr, FreePixels};
		}

		int width, height, nrChannels;
		stbi_set_flip_vertically_on_load(false); // For OpenGL coordinate system
		unsigned char* data = stbi_load(m_FilePath.string().c_str(), &width, &height, &nrChannels, 0);
		if (!data)
		{
			const char* reason = stbi_failure_reason();
			std::string error = reason ? reason : "Unknown error";
			std::cerr << "[TEXTURE] [ERROR] : Failed to load texture: " << m_FilePath << " Details: " << error
					  << std::endl;
			return {nullptr, FreePixels};
		}

		return std::unique_ptr<unsigned char[], PixelDeleter>(data, FreePixels);
	}

	unsigned int Texture::TextureID() const
	{
		return m_TextureID;
	}

	bool Texture::HasBeenLoaded() const
	{
		return m_TextureID != 0;
	}

	int Texture::Width() const
	{
		return m_Width;
	}
	int Texture::Height() const
	{
		return m_Height;
	}
	int Texture::Channels() const
	{
		return m_NbrChannels;
	}
}; // namespace onion::voxel
