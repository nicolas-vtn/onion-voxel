#pragma once

#include <filesystem>

#include <renderer/Variables.hpp>
#include <renderer/shader/shader.hpp>

namespace onion::voxel
{
	class Skybox
	{
		// ----- Constructor / Destructor -----
	  public:
		Skybox();
		~Skybox();

		// ----- Public API -----
	  public:
		void Render(const glm::mat4& view, const glm::mat4& projection);
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
	};
} // namespace onion::voxel
