#pragma once

#include <memory>
#include <string>

#include <stb_image.h>

namespace onion::voxel
{

	class Texture
	{
		// ------------ CONSTRUCTOR & DESTRUCTOR ------------
	  public:
		Texture() = delete;
		Texture(const std::string& filePath);
		~Texture();

		// ------------ LOAD ------------
	  public:
		bool LoadFromFile(const std::string& filePath);

		// ------------ BIND & UNBIND ------------
	  public:
		void Bind() const;
		void Unbind() const;

		// ------------ DELETE ------------
	  public:
		void Delete();

		// ------------ RETREVE TEXTURE DATA ------------
	  public:
		using PixelDeleter = void (*)(unsigned char*);
		std::unique_ptr<unsigned char[], PixelDeleter> GetData() const;

		// ------------ OPENGL ------------
	  private:
		std::string m_FilePath = "";
		mutable unsigned int m_TextureID = 0;

		void UploadToGPU() const;
		mutable bool m_HasBeenUploadedToGPU = false;

		// ------------- RAW TEXTURE DATA -------------
	  private:
		mutable unsigned char* m_Data = nullptr;

		// ------------ TEXTURE INFO ------------
	  private:
		int m_Width = -1;
		int m_Height = -1;
		int m_NrChannels = -1;

		// ------------ GETTERS ------------
	  public:
		unsigned int GetTextureID() const;
		bool HasBeenLoaded() const;

		int GetWidth() const;
		int GetHeight() const;
		int GetNrChannels() const;
	};
} // namespace onion::voxel
