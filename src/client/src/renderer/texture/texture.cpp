#include "texture.hpp"

#include <iostream>
#include <cstring>

#include <renderer/OpenGL.hpp>

namespace onion::voxel
{
	Texture::Texture() {}

	Texture::Texture(const std::filesystem::path& filePath, bool flipVertically)
		: m_FilePath(filePath), m_FlipVertically(flipVertically)
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

	Texture::Texture(const std::string& name,
					 const std::vector<unsigned char>& data,
					 int width,
					 int height,
					 int channels,
					 bool flipVertically)
		: m_Width(width), m_Height(height), m_NbrChannels(channels), m_FlipVertically(flipVertically)
	{
		if (data.empty())
		{
			std::cerr << "[TEXTURE] [ERROR] : Provided texture data is empty." << std::endl;
			throw std::runtime_error("Provided texture data is empty.");
		}

		size_t expectedSize = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(channels);
		if (data.size() != expectedSize)
		{
			std::cerr << "[TEXTURE] [ERROR] : Texture data size (" << data.size() << ") does not match expected size ("
					  << expectedSize << ") for dimensions (" << width << "x" << height << ") and channels ("
					  << channels << ")." << std::endl;
			throw std::runtime_error(
				"Texture data size does not match expected size for given dimensions and channels.");
		}

		unsigned char* subBuffer = new unsigned char[expectedSize];

		// Copy data to the new buffer
		std::copy(data.begin(), data.end(), subBuffer);

		auto deleter = [](unsigned char* p) { delete[] p; };
		std::unique_ptr<unsigned char[], PixelDeleter> ptr(subBuffer, deleter);

		std::filesystem::path texturePath = name;

		m_FilePath = texturePath;
		m_Data = std::move(ptr);
		m_Width = width;
		m_Height = height;
		m_NbrChannels = channels;
	}

	Texture::Texture(const std::string& name, const std::vector<unsigned char>& data, bool flipVertically)
		: m_FlipVertically(flipVertically)
	{
		if (data.empty())
		{
			throw std::runtime_error("Texture data is empty");
		}

		m_FilePath = name;

		int width, height, channels;

		stbi_set_flip_vertically_on_load(m_FlipVertically);

		unsigned char* pixels =
			stbi_load_from_memory(data.data(), static_cast<int>(data.size()), &width, &height, &channels, 0);

		if (!pixels)
		{
			const char* reason = stbi_failure_reason();
			std::string error = reason ? reason : "Unknown error";

			throw std::runtime_error("Failed to decode texture '" + name + "' from memory: " + error);
		}

		m_Width = width;
		m_Height = height;
		m_NbrChannels = channels;

		m_Data = std::unique_ptr<unsigned char[], PixelDeleter>(pixels, FreePixels);
	}

	Texture::~Texture()
	{
		if (m_Data)
		{
			m_Data.reset(); // Free raw data if it hasn't been uploaded to GPU
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
		stbi_set_flip_vertically_on_load(m_FlipVertically); // For OpenGL coordinate system
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
		m_Data = std::unique_ptr<unsigned char[], PixelDeleter>(data, FreePixels);

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
		else if (m_NbrChannels == 2)
		{
			srcFormat = GL_RG;
			internalFormat = GL_RG8;

			// Luminance + Alpha → grayscale + alpha
			GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_GREEN};
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

		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, srcFormat, GL_UNSIGNED_BYTE, m_Data.get());

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		m_Data.reset();

		m_HasBeenUploadedToGPU = true;
	}

	void Texture::Bind(uint32_t slot) const
	{

		if (!m_HasBeenUploadedToGPU)
		{
			UploadToGPU();
		}

		glActiveTexture(GL_TEXTURE0 + slot);	   // Activate texture slot
		glBindTexture(GL_TEXTURE_2D, m_TextureID); // Bind our atlas to the specified slot
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
		if (m_Data)
		{
			// Return a copy of the data to ensure the original is not modified or freed by the caller
			size_t dataSize =
				static_cast<size_t>(m_Width) * static_cast<size_t>(m_Height) * static_cast<size_t>(m_NbrChannels);
			unsigned char* dataCopy = new unsigned char[dataSize];
			std::memcpy(dataCopy, m_Data.get(), dataSize);
			return std::unique_ptr<unsigned char[], PixelDeleter>(dataCopy, FreePixels);
		}

		// Fallback : Try to load data from disk
		if (!std::filesystem::exists(m_FilePath))
		{
			std::cerr << "[TEXTURE] [ERROR] : Texture file does not exist: " << m_FilePath << std::endl;
			return {nullptr, FreePixels};
		}

		int width, height, nrChannels;
		stbi_set_flip_vertically_on_load(m_FlipVertically); // For OpenGL coordinate system
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

	Texture::Texture(const std::filesystem::path& filePath,
					 std::unique_ptr<unsigned char[], PixelDeleter> data,
					 int width,
					 int height,
					 int channels)
		: m_FilePath(filePath), m_Data(std::move(data)), m_Width(width), m_Height(height), m_NbrChannels(channels)
	{
	}

	unsigned int Texture::GetPixelFormat(int channels)
	{
		switch (channels)
		{
			case 1:
				return GL_RED;
			case 3:
				return GL_RGB;
			case 4:
				return GL_RGBA;
			default:
				throw std::runtime_error("Unsupported channel count: " + std::to_string(channels));
		}
	}

	Texture Texture::SubTexture(int x, int y, int width, int height) const
	{
		if (x < 0 || y < 0 || width <= 0 || height <= 0 || x + width > m_Width || y + height > m_Height)
		{
			std::string errorMessage = "Invalid sub-texture dimensions for texture: " + m_FilePath.string() +
				" Requested: (" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(width) + ", " +
				std::to_string(height) + ") Original: (" + std::to_string(m_Width) + ", " + std::to_string(m_Height) +
				")";
			std::cerr << "[TEXTURE] [ERROR] : " << errorMessage << std::endl;
			throw std::runtime_error(errorMessage);
		}

		// Warning : Not Optimized, for tesing only.
		auto data = GetData();

		int bytesPerPixel = m_NbrChannels;
		int rowSize = m_Width * bytesPerPixel;

		size_t size = width * height * bytesPerPixel;
		unsigned char* subBuffer = new unsigned char[size];

		for (int j = 0; j < height; j++)
		{
			int srcOffset = ((y + j) * rowSize) + (x * bytesPerPixel);
			int dstOffset = j * width * bytesPerPixel;

			std::memcpy(subBuffer + dstOffset, data.get() + srcOffset, width * bytesPerPixel);
		}

		auto deleter = [](unsigned char* p) { delete[] p; };
		std::unique_ptr<unsigned char[], PixelDeleter> ptr(subBuffer, deleter);

		std::filesystem::path subTexturePath = m_FilePath.string() + "_sub_" + std::to_string(x) + "_" +
			std::to_string(y) + "_" + std::to_string(width) + "_" + std::to_string(height);

		return Texture(subTexturePath, std::move(ptr), width, height, m_NbrChannels);
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
