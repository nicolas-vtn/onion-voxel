#include "Skybox.hpp"

#include <glm/glm.hpp>
#include <stb_image.h>

#include <iostream>

#include <renderer/OpenGL.hpp>

namespace onion::voxel
{
	Skybox::Skybox() {}

	Skybox::~Skybox() {}

	void Skybox::Render(const glm::mat4& view, const glm::mat4& projection)
	{

		if (!HasBeenInitialized())
		{
			InitSkybox(); // Initialize the skybox if not already done
		}

		glDepthMask(GL_FALSE);
		glDepthFunc(GL_LEQUAL); // skybox must be drawn last and at depth = 1.0

		m_ShaderSkybox.Use();

		glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(view));
		m_ShaderSkybox.setMat4("view", viewNoTranslation);
		m_ShaderSkybox.setMat4("projection", projection);

		glBindVertexArray(m_VAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_TextureID);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);

		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LESS); // reset
	}

	void Skybox::ReloadTextures(const CubeMapData& cubeMap)
	{
		const std::vector<std::vector<uint8_t>> faces = {cubeMap.RightFaceData,
														 cubeMap.LeftFaceData,
														 cubeMap.TopFaceData,
														 cubeMap.BottomFaceData,
														 cubeMap.FrontFaceData,
														 cubeMap.BackFaceData};

		if (m_TextureID != 0)
		{
			glDeleteTextures(1, &m_TextureID);
		}

		glGenTextures(1, &m_TextureID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_TextureID);

		stbi_set_flip_vertically_on_load(false);

		int width, height, nrChannels;

		for (unsigned int i = 0; i < faces.size(); i++)
		{
			unsigned char* pixels = stbi_load_from_memory(
				faces[i].data(), static_cast<int>(faces[i].size()), &width, &height, &nrChannels, 0);

			if (!pixels)
			{
				const char* reason = stbi_failure_reason();
				std::string error = reason ? reason : "Unknown error";

				throw std::runtime_error("Failed to decode CubeMap texture from memory: " + error);
			}

			GLenum srcFormat = Texture::GetPixelFormat(nrChannels);

			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
						 0,
						 srcFormat,
						 width,
						 height,
						 0,
						 srcFormat,
						 GL_UNSIGNED_BYTE,
						 pixels);
			stbi_image_free(pixels);
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		// Unbind the texture
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	}

	void Skybox::ReloadTextures(const CubeMapTextures& cubeMap)
	{
		const std::vector<std::string> faces = {cubeMap.RightFaceTexture,
												cubeMap.LeftFaceTexture,
												cubeMap.TopFaceTexture,
												cubeMap.BottomFaceTexture,
												cubeMap.FrontFaceTexture,
												cubeMap.BackFaceTexture};

		if (m_TextureID != 0)
		{
			glDeleteTextures(1, &m_TextureID);
		}

		glGenTextures(1, &m_TextureID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_TextureID);

		int width, height, nrChannels;

		stbi_set_flip_vertically_on_load(false);

		for (unsigned int i = 0; i < faces.size(); i++)
		{
			unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
			if (data)
			{
				GLenum srcFormat = Texture::GetPixelFormat(nrChannels);

				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
							 0,
							 srcFormat,
							 width,
							 height,
							 0,
							 srcFormat,
							 GL_UNSIGNED_BYTE,
							 data);
				stbi_image_free(data);
			}
			else
			{
				std::cerr << "Failed to load cubemap texture at " << faces[i] << std::endl;
				stbi_image_free(data);
			}
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		// Unbind the texture
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	}

	void Skybox::Unload()
	{
		if (m_HasBeenInitialized)
		{
			glDeleteVertexArrays(1, &m_VAO);
			glDeleteBuffers(1, &m_VBO);
			glDeleteTextures(1, &m_TextureID);

			m_ShaderSkybox.Delete();

			m_HasBeenInitialized = false;
		}
	}

	void Skybox::InitSkybox()
	{

		if (HasBeenInitialized())
		{
			return; // Already initialized
		}

		const Vertex skyboxVertices[] = {
			// Front face
			{-1.f, 1.f, -1.f},
			{-1.f, -1.f, -1.f},
			{1.f, -1.f, -1.f},
			{1.f, -1.f, -1.f},
			{1.f, 1.f, -1.f},
			{-1.f, 1.f, -1.f},

			// Back face
			{-1.f, -1.f, 1.f},
			{-1.f, 1.f, 1.f},
			{1.f, 1.f, 1.f},
			{1.f, 1.f, 1.f},
			{1.f, -1.f, 1.f},
			{-1.f, -1.f, 1.f},

			// Left face
			{-1.f, 1.f, 1.f},
			{-1.f, 1.f, -1.f},
			{-1.f, -1.f, -1.f},
			{-1.f, -1.f, -1.f},
			{-1.f, -1.f, 1.f},
			{-1.f, 1.f, 1.f},

			// Right face
			{1.f, 1.f, -1.f},
			{1.f, 1.f, 1.f},
			{1.f, -1.f, 1.f},
			{1.f, -1.f, 1.f},
			{1.f, -1.f, -1.f},
			{1.f, 1.f, -1.f},

			// Top face
			{-1.f, 1.f, -1.f},
			{-1.f, 1.f, 1.f},
			{1.f, 1.f, 1.f},
			{1.f, 1.f, 1.f},
			{1.f, 1.f, -1.f},
			{-1.f, 1.f, -1.f},

			// Bottom face
			{-1.f, -1.f, -1.f},
			{1.f, -1.f, -1.f},
			{1.f, -1.f, 1.f},
			{1.f, -1.f, 1.f},
			{-1.f, -1.f, 1.f},
			{-1.f, -1.f, -1.f},
		};

		glGenVertexArrays(1, &m_VAO);
		glGenBuffers(1, &m_VBO);
		glBindVertexArray(m_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) 0);

		// Set the shader's texture unit to 0 (the default for GL_TEXTURE0)
		m_ShaderSkybox.Use();
		m_ShaderSkybox.setInt("skybox", 0);

		m_HasBeenInitialized = true;
	}

	bool Skybox::HasBeenInitialized() const
	{
		return m_HasBeenInitialized;
	}

	void Skybox::LoadTextures()
	{

		const std::vector<std::string> faces = {
			(GetTexturesPath() / "skybox" / "right.bmp").string(),	// +X
			(GetTexturesPath() / "skybox" / "left.bmp").string(),	// -X
			(GetTexturesPath() / "skybox" / "top.bmp").string(),	// +Y
			(GetTexturesPath() / "skybox" / "bottom.bmp").string(), // -Y
			(GetTexturesPath() / "skybox" / "front.bmp").string(),	// +Z
			(GetTexturesPath() / "skybox" / "back.bmp").string()	// -Z
		};

		unsigned int textureID;
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

		int width, height, nrChannels;

		stbi_set_flip_vertically_on_load(false);

		for (unsigned int i = 0; i < faces.size(); i++)
		{
			unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
			if (data)
			{
				glTexImage2D(
					GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
				stbi_image_free(data);
			}
			else
			{
				std::cerr << "Failed to load cubemap texture at " << faces[i] << std::endl;
				stbi_image_free(data);
			}
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		m_TextureID = textureID; // Store the texture ID for later use
	}
} // namespace onion::voxel
