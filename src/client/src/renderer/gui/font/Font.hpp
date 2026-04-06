#pragma once

#include <glm/glm.hpp>

#include <filesystem>
#include <string>
#include <vector>

#include <renderer/OpenGL.hpp>

#include <renderer/EngineContext.hpp>
#include <renderer/shader/shader.hpp>
#include <renderer/texture/texture.hpp>

namespace onion::voxel
{
	class Font
	{
		// ----- Enums -----
	  public:
		enum class eTextAlignment
		{
			Left,
			Center,
			Right
		};

		enum class eColor : uint8_t
		{
			Black = '0',
			DarkBlue = '1',
			DarkGreen = '2',
			DarkAqua = '3',
			DarkRed = '4',
			DarkPurple = '5',
			Gold = '6',
			Gray = '7',
			DarkGray = '8',
			Blue = '9',
			Green = 'a',
			Aqua = 'b',
			Red = 'c',
			LightPurple = 'd',
			Yellow = 'e',
			White = 'f'
		};

		enum class eStyle : uint8_t
		{
			Bold = 'l',
			Strikethrough = 'm',
			Underline = 'n',
			Italic = 'o',
			Reset = 'r'
		};

		struct TextFormat
		{
			eColor color = eColor::White;
			bool bold = false;
			bool strikethrough = false;
			bool underline = false;
			bool italic = false;
		};

		struct TextSegment
		{
			std::string text;
			TextFormat format;
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

		// ----- Helper Methods -----
	  public:
		static std::string ColorToString(eColor color);
		static std::string GetColorTag(eColor color);
		static std::string StyleToString(eStyle style);
		static std::string GetStyleTag(eStyle style);

		static std::string GetFormatTag(const TextFormat& format);
		static std::string FormatText(const std::string& text, const TextFormat& format);

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
		/// @param backgroundColor The background color of the text
		void RenderText(const std::string& text,
						eTextAlignment alignment,
						const glm::vec2& position,
						float textHeightPx,
						const glm::vec3& color,
						float zOffset = 0.0f,
						float rotationDegrees = 0.0f,
						const glm::vec4& backgroundColor = glm::vec4(0.0f));

		void RenderText(const std::string& text,
						eTextAlignment alignment,
						const glm::vec2& position,
						float textHeightPx,
						float zOffset = 0.0f,
						float rotationDegrees = 0.0f,
						bool renderShadow = true,
						const glm::vec4& backgroundColor = glm::vec4(0.0f));

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

		struct VertexBackground
		{
			glm::vec3 position;
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

		GLuint m_VAO_Background = 0;
		GLuint m_VBO_Background = 0;

		std::vector<Vertex> m_Vertices;
		std::vector<VertexBackground> m_VerticesBackground;

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

		// ----- Partial Rendering -----
	  private:
		glm::ivec2 RenderPartialText(const std::string& text,
									 const glm::vec3& color,
									 const glm::vec2& position,
									 float textHeightPx,
									 float zOffset,
									 float rotationDegrees,
									 const glm::vec4& backgroundColor);

		// ----- Static Resources -----
	  private:
		static glm::mat4 s_ProjectionMatrix;
		static Shader s_ShaderFont;
		static Shader s_ShaderBackground;

		// ----- Helper Methods -----
	  private:
		static std::vector<Font::TextSegment> SegmentText(const std::string& text);

		// ----- Static Private Members -----
	  private:
		static const std::unordered_map<eColor, glm::vec3> s_ForegroundColorMap;
		static const std::unordered_map<eColor, glm::vec3> s_BackgroundColorMap;
	};
} // namespace onion::voxel
