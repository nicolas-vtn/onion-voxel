#pragma once

#include "../../shader/shader.hpp"
#include "../../texture/texture.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace onion::voxel
{
	class Font
	{
	  public:
		Font(const std::string& fontFilePath, int atlasCols, int atlasRows);
		~Font();

		void Load();
		void Unload();

		static void SetProjectionMatrix(const glm::mat4& projection);

		void RenderText(const std::string& text, float x, float y, float scale, const glm::vec3& color);
		glm::vec2 MeasureText(const std::string& text, float scale) const;

	  private:
		struct Vertex
		{
			float posX, posY, posZ;
			float texX, texY;
		};

		void GenerateBuffers();
		void DeleteBuffers();

	  private:
		struct Glyph
		{
			float advance;
			float width;
			float height;
			float u0, v0, u1, v1; // Texture coordinates
		};

	  private:
		std::string m_FontFilePath;
		Texture m_TextureAtlas;
		int m_AtlasCols = 16;
		int m_AtlasRows = 16;

		Glyph m_Glyphs[256]{};
		void InitializeGlyphs();

		static glm::mat4 s_ProjectionMatrix;

		GLuint m_VAO = 0;
		GLuint m_VBO = 0;

		std::vector<Vertex> m_Vertices;

	  private:
		static Shader m_ShaderFont;
	};
} // namespace onion::voxel
