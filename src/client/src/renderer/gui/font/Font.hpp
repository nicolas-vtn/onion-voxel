#pragma once

#include "../../shader/shader.hpp"
#include "../../texture/texture.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace onion::voxel
{
	class Font
	{
		// ----- Constructor / Destructor -----
	  public:
		Font(const std::filesystem::path& fontFilePath, int atlasCols, int atlasRows);
		~Font();

		// ----- Static Initialization / Shutdown -----
	  public:
		static void StaticInitialize();
		static void StaticShutdown();

		// ----- Public API -----
	  public:
		void Load();
		void Unload();

		static void SetProjectionMatrix(const glm::mat4& projection);

		void RenderText(const std::string& text, float x, float y, float scale, const glm::vec3& color);
		glm::vec2 MeasureText(const std::string& text, float scale) const;

		// ----- Private Structs -----
	  private:
		struct Vertex
		{
			float posX, posY, posZ;
			float texX, texY;
		};

		struct Glyph
		{
			float advance;
			float width;
			float height;
			float u0, v0, u1, v1; // Texture coordinates
		};

		// ----- OpenGL ------
		GLuint m_VAO = 0;
		GLuint m_VBO = 0;

		std::vector<Vertex> m_Vertices;

		void GenerateBuffers();
		void DeleteBuffers();

		// ----- Font Data -----
	  private:
		std::filesystem::path m_FontFilePath;
		Texture m_TextureAtlas;
		const int m_AtlasCols = 16;
		const int m_AtlasRows = 16;

		// ----- Glyph Data -----
		Glyph m_Glyphs[256]{};
		void InitializeGlyphs();

		// ----- Static Resources -----
	  private:
		static glm::mat4 s_ProjectionMatrix;
		static Shader s_ShaderFont;
	};
} // namespace onion::voxel
