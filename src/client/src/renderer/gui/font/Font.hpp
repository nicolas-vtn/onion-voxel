#pragma once

#include <renderer/EngineContext.hpp>

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
	  public:
		enum class eTextAlignment
		{
			Left,
			Center,
			Right
		};

		// ----- Constructor / Destructor -----
	  public:
		Font(const std::filesystem::path& fontFilePath, int atlasCols, int atlasRows);
		~Font();

		// ----- Static Initialization / Shutdown -----
	  public:
		/// @brief Initializes static resources for the Font class, such as shaders. This should be called once before creating any Font instances.
		static void StaticInitialize();
		/// @brief Shuts down static resources for the Font class, such as shaders. This should be called once after all Font instances are destroyed.
		static void StaticShutdown();

		// ----- Public API -----
	  public:
		/// @brief Loads the font texture and initializes OpenGL buffers. This should be called before rendering any text with this font.
		void Load();
		/// @brief Deletes the font texture and OpenGL buffers. This should be called when the font is no longer needed to free up resources.
		void Unload();
		/// @brief Reloads the font textures.
		void Reload();

		/// @brief Sets the projection matrix to be used for rendering text. This should be called whenever the screen size changes to update the projection matrix accordingly.
		static void SetProjectionMatrix(const glm::mat4& projection);

		//void RenderText(const std::string& text, float x, float y, float textHeightPx, const glm::vec3& color);

		/// @brief Renders the given text at the specified position with the specified height and color. The text is aligned based on the given alignment parameter, and can be optionally rotated by a specified angle in degrees and offset in the Z direction.
		/// @param text The text to render
		/// @param alignment The alignment of the text (Left, Center, Right)
		/// @param position The position to render the text (center for Center alignment, left-center edge for Left alignment, right-center edge for Right alignment)
		/// @param textHeightPx The height of the text in pixels
		/// @param color The color of the text
		/// @param rotationDegrees The rotation of the text in degrees
		/// @param zOffset The offset of the text in the Z direction
		void RenderText(const std::string& text,
						eTextAlignment alignment,
						const glm::vec2& position,
						float textHeightPx,
						const glm::vec3& color,
						float zOffset = 0.0f,
						float rotationDegrees = 0.0f);

		/// @brief Gets the size of the given text when rendered with the specified height. This can be used to calculate the position to render the text based on the desired alignment.
		glm::vec2 MeasureText(const std::string& text, float textHeightPx) const;

		/// @brief Gets the size of a single glyph in pixels.
		glm::ivec2 GetGlyphSize() const;

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
		glm::ivec2 m_GlyphSize{0, 0};

		// ----- Glyph Data -----
		Glyph m_Glyphs[256]{};
		void InitializeGlyphs();

		// ----- Static Resources -----
	  private:
		static glm::mat4 s_ProjectionMatrix;
		static Shader s_ShaderFont;
	};
} // namespace onion::voxel
