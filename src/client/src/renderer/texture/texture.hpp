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

		// ------------ RETREVE TEXTURE DATA ------------
	  public:
		using PixelDeleter = void (*)(unsigned char*);
		/// @brief Retrieves the raw texture data from the file. The data is managed by a unique_ptr with a custom deleter.
		std::unique_ptr<unsigned char[], PixelDeleter> GetData() const;

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
		mutable unsigned char* m_Data = nullptr;

		// ------------ TEXTURE INFO ------------
	  private:
		/// @brief The width of the texture in pixels.
		int m_Width = -1;
		/// @brief The height of the texture in pixels.
		int m_Height = -1;
		/// @brief The number of color channels in the texture (e.g., 3 for RGB, 4 for RGBA).
		int m_NbrChannels = -1;

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
	};
} // namespace onion::voxel
