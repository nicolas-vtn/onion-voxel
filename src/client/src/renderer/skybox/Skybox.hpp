#pragma once

#include <filesystem>
#include <vector>

#include <renderer/Variables.hpp>
#include <renderer/shader/shader.hpp>
#include <renderer/texture/texture.hpp>

namespace onion::voxel
{
	class Skybox
	{
		// ----- Structs -----
	  public:
		struct CubeMapData
		{
			std::vector<uint8_t> RightFaceData;
			std::vector<uint8_t> LeftFaceData;
			std::vector<uint8_t> TopFaceData;
			std::vector<uint8_t> BottomFaceData;
			std::vector<uint8_t> FrontFaceData;
			std::vector<uint8_t> BackFaceData;
		};

		struct CubeMapTextures
		{
			std::string RightFaceTexture;
			std::string LeftFaceTexture;
			std::string TopFaceTexture;
			std::string BottomFaceTexture;
			std::string FrontFaceTexture;
			std::string BackFaceTexture;
		};

		// ----- Constructor / Destructor -----
	  public:
		Skybox();
		~Skybox();

		// ----- Public API -----
	  public:
		void Render(const glm::mat4& view, const glm::mat4& projection);
		void ReloadTextures(const CubeMapData& cubeMap);
		void ReloadTextures(const CubeMapTextures& cubeMap);
		void Unload();

		// ----- OpenGL Resources -----
	  private:
		unsigned int m_TextureID = 0;
		unsigned int m_VAO = 0, m_VBO = 0;

		Shader m_ShaderSkybox{GetAssetsPath() / "shaders" / "skybox.vert", GetAssetsPath() / "shaders" / "skybox.frag"};

		// ----- Initialization -----
	  private:
		void InitSkybox();
		bool HasBeenInitialized() const;

		void LoadTextures();

		bool m_HasBeenInitialized = false;

		// ----- Private Structs -----
	  private:
		struct Vertex
		{
			float x, y, z;
		};
	};
} // namespace onion::voxel
