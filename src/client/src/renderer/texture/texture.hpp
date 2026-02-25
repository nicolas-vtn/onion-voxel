#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include <stb_image.h>

namespace onion::voxel
{

	class Texture
	{
		// ------------ CONSTRUCTOR & DESTRUCTOR ------------
	  public:
		/// @brief Default constructor. Creates an empty texture object that can be loaded later.
		Texture();
		/// @brief Constructs a texture object and loads the texture from the specified file path.
		/// @param filePath The path to the texture file to load.
		Texture(const std::filesystem::path& filePath);
		~Texture();

		Texture(Texture&&) noexcept = default;
		Texture& operator=(Texture&&) noexcept = default;

		Texture(const Texture&) = delete;
		Texture& operator=(const Texture&) = delete;

		// ------------ LOAD ------------
	  public:
		/// @brief Loads a texture from the specified file path. If a texture was already loaded, it will be replaced.
		bool LoadFromFile(const std::filesystem::path& filePath);

		// ------------ BIND & UNBIND ------------
	  public:
		/// @brief Binds the texture to the current OpenGL context, making it active for rendering.
		void Bind() const;
		/// @brief Unbinds the texture from the current OpenGL context.
		void Unbind() const;

		// ------------ DELETE ------------
	  public:
		/// @brief Deletes the texture from the GPU, freeing up resources. After calling this, the texture object should not be used unless reloaded.
		void Delete();

		// ------------ SUB TEXTURE CREATION ------------
	  public:
		/// @brief Creates a new Texture object that represents a sub-region of the current texture. This create a new OpenGL texture.
		/// @param x The x-coordinate of the top-left corner of the sub-region.
		/// @param y The y-coordinate of the top-left corner of the sub-region.
		/// @param width The width of the sub-region.
		/// @param height The height of the sub-region.
		/// @return A new Texture object representing the sub-region.
		Texture SubTexture(int x, int y, int width, int height) const;

		// ------------ GETTERS ------------
	  public:
		/// @brief Retrieves the OpenGL texture ID.
		unsigned int TextureID() const;
		/// @brief Checks if the texture has been uploaded to the GPU and is ready for use.
		bool HasBeenLoaded() const;

		/// @brief Retrieves the width of the texture in pixels.
		int Width() const;
		/// @brief Retrieves the height of the texture in pixels.
		int Height() const;
		/// @brief Retrieves the number of color channels in the texture.
		int Channels() const;

		// ------------ RETREVE TEXTURE DATA ------------
	  public:
		using PixelDeleter = void (*)(unsigned char*);
		/// @brief Retrieves the raw texture data from the file. The data is managed by a unique_ptr with a custom deleter.
		std::unique_ptr<unsigned char[], PixelDeleter> GetData() const;

		// ------------ PRIVATE CONSTRUCTOR ------------
	  private:
		/// @brief Private constructor used internally for creating sub-textures from raw data. This is not intended to be used directly by external code.
		/// @param filePath The file path associated with the texture. This is used for error reporting and debugging purposes, but does not need to correspond to an actual file since the data is provided directly.
		/// @param data A unique_ptr to the raw pixel data for the texture. The data should be in a format compatible with OpenGL (e.g., RGBA).
		/// @param width The width of the texture in pixels.
		/// @param height The height of the texture in pixels.
		/// @param channels The number of color channels in the texture.
		Texture(const std::filesystem::path& filePath,
				std::unique_ptr<unsigned char[], PixelDeleter> data,
				int width,
				int height,
				int channels);

		// ------------ OPENGL ------------
	  private:
		/// @brief The file path of the texture.
		std::filesystem::path m_FilePath;
		/// @brief The OpenGL texture ID. This is generated when the texture is uploaded to the GPU.
		mutable unsigned int m_TextureID = 0;

		/// @brief Uploads the texture data to the GPU. This is called internally when binding the texture for the first time. After uploading, the raw data is freed.
		void UploadToGPU() const;
		/// @brief Flag indicating whether the texture has been uploaded to the GPU.
		mutable bool m_HasBeenUploadedToGPU = false;

		// ------------- RAW TEXTURE DATA -------------
	  private:
		/// @brief Pointer to the raw texture data loaded from the file. This is stored temporarily until the data is uploaded to the GPU, after which it is freed.
		mutable std::unique_ptr<unsigned char[], PixelDeleter> m_Data{nullptr, FreePixels};

		// ------------ TEXTURE INFO ------------
	  private:
		/// @brief The width of the texture in pixels.
		int m_Width = -1;
		/// @brief The height of the texture in pixels.
		int m_Height = -1;
		/// @brief The number of color channels in the texture (e.g., 3 for RGB, 4 for RGBA).
		int m_NbrChannels = -1;

		// ------------ STATIC HELPERS ------------
	  private:
		static inline void FreePixels(unsigned char* ptr)
		{
			if (ptr)
			{
				stbi_image_free(ptr);
			}
		}
	};
} // namespace onion::voxel
