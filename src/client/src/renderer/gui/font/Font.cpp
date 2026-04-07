#include "font.hpp"

#include <fstream>

#include <glm/gtc/matrix_transform.hpp>

#include <renderer/gui/colored_background/ColoredBackground.hpp>

namespace onion::voxel
{

	// -------- Static Member Definitions --------

	Shader Font::s_ShaderFont(AssetsManager::GetShadersDirectory() / "font.vert",
							  AssetsManager::GetShadersDirectory() / "font.frag");

	glm::mat4 Font::s_ProjectionMatrix{1.0f};

	constexpr float c(float v)
	{
		return v / 255.f;
	}

	const std::unordered_map<Font::eColor, glm::vec3> Font::s_TextColorMap = {
		{eColor::Black, {c(0.f), c(0.f), c(0.f)}},
		{eColor::DarkBlue, {c(0.f), c(0.f), c(170.f)}},
		{eColor::DarkGreen, {c(0.f), c(170.f), c(0.f)}},
		{eColor::DarkAqua, {c(0.f), c(170.f), c(170.f)}},
		{eColor::DarkRed, {c(170.f), c(0.f), c(0.f)}},
		{eColor::DarkPurple, {c(170.f), c(0.f), c(170.f)}},
		{eColor::Gold, {c(255.f), c(170.f), c(0.f)}},
		{eColor::Gray, {c(170.f), c(170.f), c(170.f)}},
		{eColor::DarkGray, {c(85.f), c(85.f), c(85.f)}},
		{eColor::Blue, {c(85.f), c(85.f), c(255.f)}},
		{eColor::Green, {c(85.f), c(255.f), c(85.f)}},
		{eColor::Aqua, {c(85.f), c(255.f), c(255.f)}},
		{eColor::Red, {c(255.f), c(85.f), c(85.f)}},
		{eColor::LightPurple, {c(255.f), c(85.f), c(255.f)}},
		{eColor::Yellow, {c(255.f), c(255.f), c(85.f)}},
		{eColor::White, {c(255.f), c(255.f), c(255.f)}}};

	const std::unordered_map<Font::eColor, glm::vec3> Font::s_ShadowColorMap = {
		{eColor::Black, {c(0.f), c(0.f), c(0.f)}},
		{eColor::DarkBlue, {c(0.f), c(0.f), c(42.f)}},
		{eColor::DarkGreen, {c(0.f), c(42.f), c(0.f)}},
		{eColor::DarkAqua, {c(0.f), c(42.f), c(42.f)}},
		{eColor::DarkRed, {c(42.f), c(0.f), c(0.f)}},
		{eColor::DarkPurple, {c(42.f), c(0.f), c(42.f)}},
		{eColor::Gold, {c(42.f), c(42.f), c(0.f)}},
		{eColor::Gray, {c(42.f), c(42.f), c(42.f)}},
		{eColor::DarkGray, {c(21.f), c(21.f), c(21.f)}},
		{eColor::Blue, {c(21.f), c(21.f), c(63.f)}},
		{eColor::Green, {c(21.f), c(63.f), c(21.f)}},
		{eColor::Aqua, {c(21.f), c(63.f), c(63.f)}},
		{eColor::Red, {c(63.f), c(21.f), c(21.f)}},
		{eColor::LightPurple, {c(63.f), c(21.f), c(63.f)}},
		{eColor::Yellow, {c(63.f), c(63.f), c(21.f)}},
		{eColor::White, {c(63.f), c(63.f), c(63.f)}}};

	// -------- Constructor / Destructor --------

	Font::Font(const std::filesystem::path& fontFilePath, int atlasCols, int atlasRows) {}

	Font::~Font() {}

	// -------- Public API --------

	void Font::Load()
	{
		auto& assetsManager = EngineContext::Get().Assets;
		std::filesystem::path fontProvidersPath = std::filesystem::path("font") / "include" / "default.json";
		InitializeUnicodeGlyphs(assetsManager->GetAssetsDirectory() / fontProvidersPath);

		s_ShaderFont.Use();
	}

	void Font::Unload()
	{
		DeleteGlyphProviders();
	}

	void Font::Reload()
	{
		Unload();
		Load();
	}

	void Font::StaticInitialize() {}

	void Font::StaticShutdown()
	{
		s_ShaderFont.Delete();
	}

	std::string Font::ColorToString(eColor color)
	{
		switch (color)
		{
			case eColor::Black:
				return "black";
			case eColor::DarkBlue:
				return "dark_blue";
			case eColor::DarkGreen:
				return "dark_green";
			case eColor::DarkAqua:
				return "dark_aqua";
			case eColor::DarkRed:
				return "dark_red";
			case eColor::DarkPurple:
				return "dark_purple";
			case eColor::Gold:
				return "gold";
			case eColor::Gray:
				return "gray";
			case eColor::DarkGray:
				return "dark_gray";
			case eColor::Blue:
				return "blue";
			case eColor::Green:
				return "green";
			case eColor::Aqua:
				return "aqua";
			case eColor::Red:
				return "red";
			case eColor::LightPurple:
				return "light_purple";
			case eColor::Yellow:
				return "yellow";
			case eColor::White:
				return "white";
			default:
				return "unknown_color";
		}
	}

	std::string Font::GetColorTag(eColor color)
	{
		return std::string("§") + static_cast<char>(color);
	}

	std::string Font::StyleToString(eStyle style)
	{
		switch (style)
		{
			case eStyle::Bold:
				return "bold";
			case eStyle::Strikethrough:
				return "strikethrough";
			case eStyle::Underline:
				return "underline";
			case eStyle::Italic:
				return "italic";
			case eStyle::Reset:
				return "reset";
			default:
				return "unknown_style";
		}
	}

	std::string Font::GetStyleTag(eStyle style)
	{
		return std::string("§") + static_cast<char>(style);
	}

	std::string Font::GetFormatTag(eColor color, const TextFormat& format)
	{
		std::string tag;
		tag += GetColorTag(color);
		if (format.bold)
			tag += GetStyleTag(eStyle::Bold);
		if (format.strikethrough)
			tag += GetStyleTag(eStyle::Strikethrough);
		if (format.underline)
			tag += GetStyleTag(eStyle::Underline);
		if (format.italic)
			tag += GetStyleTag(eStyle::Italic);
		return tag;
	}

	std::string Font::FormatText(const std::string& text, eColor color, const TextFormat& format)
	{
		return GetFormatTag(color, format) + text + GetStyleTag(eStyle::Reset);
	}

	void Font::SetProjectionMatrix(const glm::mat4& projection)
	{
		s_ProjectionMatrix = projection;
		s_ShaderFont.Use();
		s_ShaderFont.setMat4("uProjection", s_ProjectionMatrix);
	}

	void Font::RenderText(const std::string& text,
						  eTextAlignment alignment,
						  const glm::vec2& position,
						  float textHeightPx,
						  float zOffset,
						  float rotationDegrees,
						  bool renderShadow)
	{
		std::vector<TextSegment> segments = SegmentText(text);

		glm::ivec2 size{0, 0};
		for (const TextSegment& segment : segments)
		{
			glm::vec2 segmentSize = MeasureText(segment.text, textHeightPx);
			size.x += static_cast<int>(round(segmentSize.x));
			size.y = std::max(size.y, static_cast<int>(round(segmentSize.y)));
		}

		// Calculate Starting position based on alignment
		float startX = 0.f, startY = 0.f;
		glm::vec2 topLeftCorner{};
		glm::vec2 bottomRightCorner{};
		switch (alignment)
		{
			case eTextAlignment::Left:
				startX = position.x;
				startY = position.y - size.y * 0.5f; // Center vertically
				topLeftCorner = {position.x, position.y - size.y * 0.5f};
				bottomRightCorner = {position.x + size.x, position.y + size.y * 0.5f};
				break;

			case eTextAlignment::Center:
				startX = position.x - size.x * 0.5f;
				startY = position.y - size.y * 0.5f; // Center vertically
				topLeftCorner = {position.x - size.x * 0.5f, position.y - size.y * 0.5f};
				bottomRightCorner = {position.x + size.x * 0.5f, position.y + size.y * 0.5f};
				break;

			case eTextAlignment::Right:
				startX = position.x - size.x;
				startY = position.y - size.y * 0.5f; // Center vertically
				topLeftCorner = {static_cast<int>(position.x - size.x), static_cast<int>(position.y - size.y * 0.5f)};
				bottomRightCorner = {static_cast<int>(position.x), static_cast<int>(position.y + size.y * 0.5f)};
				break;
		}

		glm::vec2 pivot = {topLeftCorner.x + size.x * 0.5f, topLeftCorner.y + size.y * 0.5f};

		glm::vec2 currentPos = {startX, startY};
		if (renderShadow)
		{
			currentPos = {startX, startY};
			glm::ivec2 shadowOffset{static_cast<int>(round(textHeightPx / 8.f)),
									static_cast<int>(round(textHeightPx / 8.f))};
			currentPos += shadowOffset;
			for (const TextSegment& segment : segments)
			{
				const glm::vec3& shadowColor = s_ShadowColorMap.at(segment.color);
				currentPos = RenderPartialText(segment.text,
											   shadowColor,
											   currentPos,
											   segment.format,
											   textHeightPx,
											   zOffset - 0.01f,
											   rotationDegrees,
											   pivot);
			}
		}

		currentPos = {startX, startY};
		for (const TextSegment& segment : segments)
		{
			const glm::vec3& foregroundColor = s_TextColorMap.at(segment.color);
			currentPos = RenderPartialText(segment.text,
										   foregroundColor,
										   currentPos,
										   segment.format,
										   textHeightPx,
										   zOffset,
										   rotationDegrees,
										   pivot);
		}
	}

	void Font::RenderText(const std::string& text,
						  eTextAlignment alignment,
						  const glm::vec2& position,
						  const glm::vec3& textColor,
						  const glm::vec3& shadowColor,
						  float textHeightPx,
						  const Font::TextFormat& format,
						  float zOffset,
						  float rotationDegrees,
						  bool renderShadow)
	{
		glm::ivec2 size = MeasureText(text, textHeightPx);

		// Calculate Starting position based on alignment
		float startX = 0.f, startY = 0.f;
		glm::vec2 topLeftCorner{};
		glm::vec2 bottomRightCorner{};
		switch (alignment)
		{
			case eTextAlignment::Left:
				startX = position.x;
				startY = position.y - size.y * 0.5f; // Center vertically
				topLeftCorner = {position.x, position.y - size.y * 0.5f};
				bottomRightCorner = {position.x + size.x, position.y + size.y * 0.5f};
				break;

			case eTextAlignment::Center:
				startX = position.x - size.x * 0.5f;
				startY = position.y - size.y * 0.5f; // Center vertically
				topLeftCorner = {position.x - size.x * 0.5f, position.y - size.y * 0.5f};
				bottomRightCorner = {position.x + size.x * 0.5f, position.y + size.y * 0.5f};
				break;

			case eTextAlignment::Right:
				startX = position.x - size.x;
				startY = position.y - size.y * 0.5f; // Center vertically
				topLeftCorner = {static_cast<int>(position.x - size.x), static_cast<int>(position.y - size.y * 0.5f)};
				bottomRightCorner = {static_cast<int>(position.x), static_cast<int>(position.y + size.y * 0.5f)};
				break;
		}

		glm::vec2 pivot = {topLeftCorner.x + size.x * 0.5f, topLeftCorner.y + size.y * 0.5f};

		glm::vec2 currentPos = {startX, startY};
		if (renderShadow)
		{
			currentPos = {startX, startY};
			glm::ivec2 shadowOffset{static_cast<int>(round(textHeightPx / 8.f)),
									static_cast<int>(round(textHeightPx / 8.f))};
			currentPos += shadowOffset;

			currentPos = RenderPartialText(
				text, shadowColor, currentPos, format, textHeightPx, zOffset - 0.01f, rotationDegrees, pivot);
		}

		currentPos = {startX, startY};
		currentPos =
			RenderPartialText(text, textColor, currentPos, format, textHeightPx, zOffset, rotationDegrees, pivot);
	}

	glm::vec2 Font::MeasureText(const std::string& text, float textHeightPx) const
	{
		if (text.empty())
			return {0.f, 0.f};

		std::u32string unicodeText = Utf8ToUtf32(text);

		const float refGlyphHeight = m_UnicodeGlyphs.at('A').height;

		float width = 0.f;
		for (int i = 0; i < unicodeText.size(); i++)
		{
			float scale = textHeightPx / refGlyphHeight;
			char32_t c = unicodeText[i];

			auto it = m_UnicodeGlyphs.find(c);
			if (it != m_UnicodeGlyphs.end())
			{
				const Glyph& glyph = m_UnicodeGlyphs.at(c);
				width += glyph.advance * scale;
			}
			else
			{
				width += m_UnicodeGlyphs.at(' ').advance * scale; // Fallback to space advance for unknown characters
			}
		}

		const float scale = textHeightPx / refGlyphHeight;
		float height = refGlyphHeight * scale;

		return {width, height};
	}

	// -------- OpenGL Buffer Setup --------

	float GetGlyphAdvance(unsigned char* data,
						  int col,
						  int row,
						  int glyphPixelWidth,
						  int glyphPixelHeight,
						  int textureWidth,
						  int nbChannels)
	{
		int startX = col * glyphPixelWidth;
		int startY = row * glyphPixelHeight;

		int firstNonEmpty = glyphPixelWidth;
		int lastNonEmpty = 0;

		bool isEmpty = true;

		// Scan columns
		for (int x = 0; x < glyphPixelWidth; x++)
		{
			bool columnHasPixel = false;

			for (int y = 0; y < glyphPixelHeight; y++)
			{
				int px = startX + x;
				int py = startY + y;

				int index = (py * textureWidth + px) * nbChannels;

				unsigned char alpha = data[index + 3];

				if (alpha > 10) // threshold
				{
					columnHasPixel = true;
					isEmpty = false;
					break;
				}
			}

			if (columnHasPixel)
			{
				firstNonEmpty = std::min(firstNonEmpty, x);
				lastNonEmpty = std::max(lastNonEmpty, x);
			}
		}

		if (isEmpty)
			return glyphPixelWidth * 0.5f; // Arbitrary advance for empty glyphs

		return lastNonEmpty - firstNonEmpty + 2.f;
	}

	void Font::InitializeUnicodeGlyphs(const std::filesystem::path& fontProvidersPath)
	{
		std::ifstream file(fontProvidersPath);
		if (!file.is_open())
			throw std::runtime_error("Failed to open font providers file");

		nlohmann::json j;
		file >> j;

		FontProviders providers = j.get<FontProviders>();

		for (auto& fontProvider : providers.providers)
		{
			// Load the Texture for this provider

			// Remove all text before "minecraft:"
			const std::string resourcePrefix = "minecraft:";
			std::string relativePath = fontProvider.file;
			size_t pos = relativePath.find(resourcePrefix);
			if (pos != std::string::npos)
			{
				relativePath = relativePath.substr(pos + resourcePrefix.length());
			}

			// Create a GlyphProvider
			GlyphProvider provider;

			std::filesystem::path providerTexturePath =
				std::filesystem::path("assets") / "minecraft" / "textures" / relativePath;
			std::vector<uint8_t> textureData =
				EngineContext::Get().Assets->GetResourcePackFileBinary(providerTexturePath);
			provider.TextureGlyph = std::move(Texture(providerTexturePath.string(), textureData));
			m_GlyphProviders.push_back(std::move(provider));
			GlyphProvider& providerRef = m_GlyphProviders.back();
			Texture& providerTexture = providerRef.TextureGlyph;

			// For each character mapping, store the corresponding UV coordinates
			auto data = providerTexture.GetData();
			int width = providerTexture.Width();
			int height = providerTexture.Height();
			int nrChannels = providerTexture.Channels();

			int glyphPixelWidth = width / fontProvider.totalCols;
			int glyphPixelHeight = height / fontProvider.totalRows;

			for (auto& [charCode, glyphPos] : fontProvider.chars)
			{
				int col = glyphPos.colIndex;
				int row = glyphPos.rowIndex;

				Glyph glyph;
				glyph.glyphProvider = &providerRef;

				glyph.ascent = fontProvider.ascent;
				glyph.width = (float) glyphPixelWidth;
				glyph.height = (float) glyphPixelHeight;
				glyph.advance =
					GetGlyphAdvance(data.get(), col, row, glyphPixelWidth, glyphPixelHeight, width, nrChannels);
				float uvStepX = 1.0f / fontProvider.totalCols;
				float uvStepY = 1.0f / fontProvider.totalRows;
				glyph.u0 = col * uvStepX;
				glyph.v0 = row * uvStepY;
				glyph.u1 = glyph.u0 + uvStepX;
				glyph.v1 = glyph.v0 + uvStepY;

				m_UnicodeGlyphs[charCode] = glyph;
			}
		}

		InitiaizeGlyphProviders();
	}

	void Font::InitiaizeGlyphProviders()
	{
		for (auto& provider : m_GlyphProviders)
		{
			provider.TextureGlyph.Bind();

			glGenVertexArrays(1, &provider.VAO);
			glGenBuffers(1, &provider.VBO);

			glBindVertexArray(provider.VAO);
			glBindBuffer(GL_ARRAY_BUFFER, provider.VBO);
			glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, posX));
			glEnableVertexAttribArray(0);

			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, texX));
			glEnableVertexAttribArray(1);

			// Unbind
			glBindVertexArray(0);
		}
	}

	void Font::DeleteGlyphProviders()
	{
		for (auto& provider : m_GlyphProviders)
		{
			if (provider.VAO)
				glDeleteVertexArrays(1, &provider.VAO);
			if (provider.VBO)
				glDeleteBuffers(1, &provider.VBO);
			provider.VAO = 0;
			provider.VBO = 0;
			provider.TextureGlyph.Delete();
		}
	}

	std::vector<Font::TextSegment> Font::SegmentText(const std::string& text)
	{
		std::vector<TextSegment> segments;

		eColor currentColor = eColor::White;
		TextFormat currentFormat;
		std::string currentText;

		for (size_t i = 0; i < text.size(); ++i)
		{
			// Detect UTF-8 section sign: 0xC2 0xA7
			if (static_cast<unsigned char>(text[i]) == 0xC2 && i + 2 < text.size() &&
				static_cast<unsigned char>(text[i + 1]) == 0xA7)
			{
				// Flush accumulated text before applying new formatting
				if (!currentText.empty())
				{
					segments.push_back({currentText, currentColor, currentFormat});
					currentText.clear();
				}

				char code = static_cast<char>(std::tolower(static_cast<unsigned char>(text[i + 2])));
				i += 2; // Skip both UTF-8 bytes of § and the loop will continue after the code

				// Color code: colors also reset previous styles in Minecraft
				if ((code >= '0' && code <= '9') || (code >= 'a' && code <= 'f'))
				{
					currentFormat = TextFormat{};
					currentColor = static_cast<eColor>(code);
				}
				else
				{
					switch (code)
					{
						case 'l':
							currentFormat.bold = true;
							break;
						case 'm':
							currentFormat.strikethrough = true;
							break;
						case 'n':
							currentFormat.underline = true;
							break;
						case 'o':
							currentFormat.italic = true;
							break;
						case 'r':
							currentFormat = TextFormat{};
							break;
						default:
							// Unknown code: ignore it
							break;
					}
				}
			}
			else
			{
				currentText += text[i];
			}
		}

		if (!currentText.empty())
		{
			segments.push_back({currentText, currentColor, currentFormat});
		}

		return segments;
	}

	glm::ivec2 Font::RenderPartialText(const std::string& text,
									   const glm::vec3& color,
									   const glm::vec2& position,
									   const Font::TextFormat& textFormat,
									   float textHeightPx,
									   float zOffset,
									   float rotationDegrees,
									   const glm::vec2& pivot)
	{
		if (text.empty())
			return position;

		std::u32string unicodeText = Utf8ToUtf32(text);

		glm::vec2 size = MeasureText(text, textHeightPx);

		const float refGlyphHeight = m_UnicodeGlyphs['A'].height;

		// Build vertices
		auto DrawGlyph = [&](float offsetX_px)
		{
			float cursorX = position.x;
			float cursorY = position.y;

			for (char32_t c : unicodeText)
			{
				Glyph glyph = m_UnicodeGlyphs.at('?'); // Fallback glyph
				auto it = m_UnicodeGlyphs.find(c);
				if (it != m_UnicodeGlyphs.end())
				{
					glyph = it->second;
				}

				float scale = textHeightPx / refGlyphHeight;

				float skew = textFormat.italic ? (glyph.height * scale * 0.25f) : 0.0f;

				float x0 = cursorX + offsetX_px;
				float x1 = x0 + glyph.width * scale;

				int deltaAscent = glyph.ascent - 7;

				float y0 = cursorY - deltaAscent * scale;
				float y1 = y0 + glyph.height * scale;

				// Top edge is shifted to the right
				float x0_top = x0 + skew;
				float x1_top = x1 + skew;

				GlyphProvider* provider = glyph.glyphProvider;

				auto& vertices = provider->Vertices;

				vertices.push_back({x0_top, y0, zOffset, glyph.u0, glyph.v0});
				vertices.push_back({x1_top, y0, zOffset, glyph.u1, glyph.v0});
				vertices.push_back({x1, y1, zOffset, glyph.u1, glyph.v1});

				vertices.push_back({x0_top, y0, zOffset, glyph.u0, glyph.v0});
				vertices.push_back({x1, y1, zOffset, glyph.u1, glyph.v1});
				vertices.push_back({x0, y1, zOffset, glyph.u0, glyph.v1});

				cursorX += glyph.advance * scale;
			}
		};

		// Draw main text
		DrawGlyph(0.0f);

		// Draw bold text (simple offset)
		if (textFormat.bold)
		{
			int offsetX_px = static_cast<int>(round(textHeightPx / 8.f));
			int magicOffset = static_cast<int>(round(offsetX_px / 2.f));
			offsetX_px -= magicOffset;
			DrawGlyph(static_cast<float>(offsetX_px));
		}

		// --- Rotation Matrix ---
		glm::mat4 model(1.0f);

		if (rotationDegrees != 0.0f)
		{
			model = glm::translate(model, glm::vec3(pivot, 0.0f));
			model = glm::rotate(model, glm::radians(rotationDegrees), glm::vec3(0, 0, 1));
			model = glm::translate(model, glm::vec3(-pivot, 0.0f));
		}

		// --- Render Unicode Text ---
		for (auto& glyphProvider : m_GlyphProviders)
		{
			auto& vertices = glyphProvider.Vertices;

			if (vertices.empty())
				continue;

			glBindVertexArray(glyphProvider.VAO);
			glBindBuffer(GL_ARRAY_BUFFER, glyphProvider.VBO);

			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);

			glyphProvider.TextureGlyph.Bind();

			s_ShaderFont.Use();
			s_ShaderFont.setVec3("uTextColor", color);
			s_ShaderFont.setInt("uTexture", 0);
			s_ShaderFont.setMat4("uModel", model);

			glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));

			glBindVertexArray(0);

			vertices.clear();
		}

		// Draw Underline
		if (textFormat.underline)
		{
			int onePx = static_cast<int>(round(textHeightPx / 8.f));

			int topY = static_cast<int>(round(position.y + textHeightPx));
			int bottomY = topY + onePx;

			int leftX = static_cast<int>(round(position.x));
			int rightX = static_cast<int>(round(position.x + size.x));
			ColoredBackground::CornerOptions underlineOptions;
			underlineOptions.TopLeftCorner = {leftX, topY};
			underlineOptions.BottomRightCorner = {rightX, bottomY};
			underlineOptions.Color = glm::vec4(color, 1.0f);
			underlineOptions.ZOffset = zOffset;
			underlineOptions.RotationDegrees = rotationDegrees;

			ColoredBackground::Render(underlineOptions);
		}

		// Draw Strikethrough
		if (textFormat.strikethrough)
		{
			int onePx = static_cast<int>(round(textHeightPx / 8.f));

			int centerY = static_cast<int>(round(position.y + textHeightPx * 0.5f));
			int topY = centerY - static_cast<int>(round(onePx * 0.5f));
			int bottomY = centerY + static_cast<int>(round(onePx * 0.5f));
			int leftX = static_cast<int>(round(position.x));
			int rightX = static_cast<int>(round(position.x + size.x));
			ColoredBackground::CornerOptions strikethroughOptions;
			strikethroughOptions.TopLeftCorner = {leftX, topY};
			strikethroughOptions.BottomRightCorner = {rightX, bottomY};
			strikethroughOptions.Color = glm::vec4(color, 1.0f);
			strikethroughOptions.ZOffset = zOffset;
			strikethroughOptions.RotationDegrees = rotationDegrees;

			ColoredBackground::Render(strikethroughOptions);
		}

		return {position.x + size.x, position.y};
	}
} // namespace onion::voxel
