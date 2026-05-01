#pragma once

#include <glm/glm.hpp>

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include <renderer/OpenGL.hpp>

#include <renderer/EngineContext.hpp>
#include <renderer/shader/shader.hpp>
#include <renderer/texture/texture.hpp>

#include "FontProvider.hpp"

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
			bool bold = false;
			bool strikethrough = false;
			bool underline = false;
			bool italic = false;
		};

		struct TextSegment
		{
			std::u32string text;
			eColor color = eColor::White;
			TextFormat format{};
		};

		// ----- Constructor / Destructor -----
	  public:
		Font();
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

		static std::string GetFormatTag(eColor color, const TextFormat& format);
		static std::u32string FormatText(const std::u32string& text, eColor color, const TextFormat& format);

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

		/// @brief Renders the given text at the specified position with the specified height and color. The text is aligned based on the given alignment parameter, and can be optionally rotated by a specified angle in degrees and offset in the Z direction.
		/// @param text The text to render
		/// @param alignment The alignment of the text (Left, Center, Right)
		/// @param position The position to render the text (center for Center alignment, left-center edge for Left alignment, right-center edge for Right alignment)
		/// @param textHeightPx The height of the text in pixels
		/// @param zOffset The offset of the text in the Z direction
		/// @param rotationDegrees The rotation of the text in degrees
		/// @param renderShadow Whether to render a shadow for the text.
		void RenderText(const std::string& text,
						eTextAlignment alignment,
						const glm::vec2& position,
						float textHeightPx,
						float zOffset = 0.0f,
						float rotationDegrees = 0.0f,
						bool renderShadow = true);

		/// @brief Renders the given text at the specified position with the specified height and color. The text is aligned based on the given alignment parameter, and can be optionally rotated by a specified angle in degrees and offset in the Z direction.
		/// @param text The text to render
		/// @param alignment The alignment of the text (Left, Center, Right)
		/// @param position The position to render the text (center for Center alignment, left-center edge for Left alignment, right-center edge for Right alignment)
		/// @param textColor The color of the text
		/// @param shadowColor The color of the shadow
		/// @param textHeightPx The height of the text in pixels
		/// @param zOffset The offset of the text in the Z direction
		/// @param rotationDegrees The rotation of the text in degrees
		/// @param renderShadow Whether to render a shadow for the text.
		void RenderText(const std::string& text,
						eTextAlignment alignment,
						const glm::vec2& position,
						const glm::vec4& textColor,
						const glm::vec4& shadowColor,
						float textHeightPx,
						const Font::TextFormat& format,
						float zOffset = 0.0f,
						float rotationDegrees = 0.0f,
						bool renderShadow = true);

		/// @brief Renders the given text at the specified position with the specified height and color. The text is aligned based on the given alignment parameter, and can be optionally rotated by a specified angle in degrees and offset in the Z direction.
		/// @param text The text to render
		/// @param alignment The alignment of the text (Left, Center, Right)
		/// @param position The position to render the text (center for Center alignment, left-center edge for Left alignment, right-center edge for Right alignment)
		/// @param textHeightPx The height of the text in pixels
		/// @param zOffset The offset of the text in the Z direction
		/// @param rotationDegrees The rotation of the text in degrees
		/// @param renderShadow Whether to render a shadow for the text.
		void RenderText(const std::u32string& text,
						eTextAlignment alignment,
						const glm::vec2& position,
						float textHeightPx,
						float zOffset = 0.0f,
						float rotationDegrees = 0.0f,
						bool renderShadow = true);

		/// @brief Renders the given text at the specified position with the specified height and color. The text is aligned based on the given alignment parameter, and can be optionally rotated by a specified angle in degrees and offset in the Z direction.
		/// @param text The text to render
		/// @param alignment The alignment of the text (Left, Center, Right)
		/// @param position The position to render the text (center for Center alignment, left-center edge for Left alignment, right-center edge for Right alignment)
		/// @param textColor The color of the text
		/// @param shadowColor The color of the shadow
		/// @param textHeightPx The height of the text in pixels
		/// @param zOffset The offset of the text in the Z direction
		/// @param rotationDegrees The rotation of the text in degrees
		/// @param renderShadow Whether to render a shadow for the text.
		void RenderText(const std::u32string& text,
						eTextAlignment alignment,
						const glm::vec2& position,
						const glm::vec4& textColor,
						const glm::vec4& shadowColor,
						float textHeightPx,
						const Font::TextFormat& format,
						float zOffset = 0.0f,
						float rotationDegrees = 0.0f,
						bool renderShadow = true);

		/// @brief Gets the size of the given text when rendered with the specified height. This can be used to calculate the position to render the text based on the desired alignment.
		glm::vec2 MeasureText(const std::string& text, float textHeightPx) const;

		glm::vec2 MeasureText(const std::u32string& text, float textHeightPx) const;

		// ----- Private Structs -----
	  private:
		struct Vertex
		{
			float posX, posY, posZ;
			float texX, texY;
		};

		struct GlyphProvider
		{
			Texture TextureGlyph;
			GLuint VAO = 0;
			GLuint VBO = 0;
			std::vector<Vertex> Vertices;
		};

		struct Glyph
		{
			GlyphProvider* glyphProvider = nullptr;
			int ascent = 0;
			float advance = 0.f;
			float width = 0.f;
			float height = 0.f;
			float u0 = 0.f, v0 = 0.f, u1 = 0.f, v1 = 0.f; // Texture coordinates
		};

		// ----- Glyph Data -----
	  private:
		std::vector<GlyphProvider> m_GlyphProviders;
		std::unordered_map<uint32_t, Glyph> m_UnicodeGlyphs;

		void InitializeUnicodeGlyphs(const std::filesystem::path& fontProvidersPath);

		void InitiaizeGlyphProviders();
		void DeleteGlyphProviders();

		// ----- Partial Rendering -----
	  private:
		glm::ivec2 RenderPartialText(const std::u32string& text,
									 const glm::vec4& color,
									 const glm::vec2& position,
									 const Font::TextFormat& textFormat,
									 float textHeightPx,
									 float zOffset,
									 float rotationDegrees,
									 const glm::vec2& pivot,
									 eTextAlignment alignment,
									 float maxLineWidth);

		// ----- Static Resources -----
	  private:
		static glm::mat4 s_ProjectionMatrix;
		static Shader s_ShaderFont;

		// ----- Helper Methods -----
	  private:
		static std::vector<Font::TextSegment> SegmentText(const std::u32string& text);

		// ----- Static Private Members -----
	  private:
		static const std::unordered_map<eColor, glm::vec4> s_TextColorMap;
		static const std::unordered_map<eColor, glm::vec4> s_ShadowColorMap;
	};
} // namespace onion::voxel
