#include "font.hpp"

#include <renderer/Variables.hpp>

#include <glm/gtc/matrix_transform.hpp>

using namespace onion::voxel;

// -------- Static Member Definitions --------

Shader Font::s_ShaderFont(GetAssetsPath() / "shaders" / "font.vert", GetAssetsPath() / "shaders" / "font.frag");

glm::mat4 Font::s_ProjectionMatrix{1.0f};

// -------- Constructor / Destructor --------

Font::Font(const std::filesystem::path& fontFilePath, int atlasCols, int atlasRows)
	: m_FontFilePath(fontFilePath), m_AtlasCols(atlasCols), m_AtlasRows(atlasRows)
{
}

Font::~Font() {}

// -------- Public API --------

void Font::Load()
{
	InitializeGlyphs();

	m_TextureAtlas.Bind(); // Upload texture
	s_ShaderFont.Use();
	s_ShaderFont.setInt("uTexture", m_TextureAtlas.TextureID());
	GenerateBuffers();
}

void Font::Unload()
{
	m_TextureAtlas.Delete();
	DeleteBuffers();
}

void onion::voxel::Font::Reload()
{
	Unload();

	std::vector<unsigned char> data = EngineContext::Get().Assets->GetResourcePackFileBinary(m_FontFilePath);
	m_TextureAtlas = Texture(m_FontFilePath.string(), data);

	Load();
}

void Font::StaticInitialize() {}

void Font::StaticShutdown()
{
	s_ShaderFont.Delete();
}

void Font::SetProjectionMatrix(const glm::mat4& projection)
{
	s_ProjectionMatrix = projection;
	s_ShaderFont.Use();
	s_ShaderFont.setMat4("uProjection", s_ProjectionMatrix);
}

void onion::voxel::Font::RenderText(const std::string& text,
									eTextAlignment alignment,
									const glm::vec2& position,
									float textHeightPx,
									const glm::vec3& color,
									float zOffset,
									float rotationDegrees)
{
	if (text.empty())
		return;

	glm::vec2 size = MeasureText(text, textHeightPx);

	// Calculate Starting position based on alignment
	float startX = 0.f, startY = 0.f;
	switch (alignment)
	{
		case eTextAlignment::Left:
			startX = position.x;
			startY = position.y - size.y * 0.5f; // Center vertically
			break;

		case eTextAlignment::Center:
			startX = position.x - size.x * 0.5f;
			startY = position.y - size.y * 0.5f; // Center vertically
			break;

		case eTextAlignment::Right:
			startX = position.x - size.x;
			startY = position.y - size.y * 0.5f; // Center vertically
			break;
	}

	// Build vertices
	float cursorX = startX;
	float cursorY = startY;

	float scale = textHeightPx / m_GlyphSize.y;

	for (char c : text)
	{
		int ascii = static_cast<unsigned char>(c);
		const Glyph& glyph = m_Glyphs[ascii];

		float x0 = cursorX;
		float y0 = cursorY;
		float x1 = x0 + glyph.width * scale;
		float y1 = y0 + glyph.height * scale;

		m_Vertices.push_back({x0, y0, zOffset, glyph.u0, glyph.v0});
		m_Vertices.push_back({x1, y0, zOffset, glyph.u1, glyph.v0});
		m_Vertices.push_back({x1, y1, zOffset, glyph.u1, glyph.v1});

		m_Vertices.push_back({x0, y0, zOffset, glyph.u0, glyph.v0});
		m_Vertices.push_back({x1, y1, zOffset, glyph.u1, glyph.v1});
		m_Vertices.push_back({x0, y1, zOffset, glyph.u0, glyph.v1});

		cursorX += glyph.advance * scale;
	}

	// --- Rotation Matrix ---
	glm::mat4 model(1.0f);

	if (rotationDegrees != 0.0f)
	{
		model = glm::translate(model, glm::vec3(position, 0.0f));
		model = glm::rotate(model, glm::radians(rotationDegrees), glm::vec3(0, 0, 1));
		model = glm::translate(model, glm::vec3(-position, 0.0f));
	}

	// --- Render ---
	glBindVertexArray(m_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

	glBufferData(GL_ARRAY_BUFFER, m_Vertices.size() * sizeof(Vertex), m_Vertices.data(), GL_DYNAMIC_DRAW);

	m_TextureAtlas.Bind();

	s_ShaderFont.Use();
	s_ShaderFont.setVec3("uTextColor", color);
	s_ShaderFont.setInt("uTexture", 0);
	s_ShaderFont.setMat4("uModel", model);

	glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_Vertices.size()));

	glBindVertexArray(0);

	m_Vertices.clear();
}

glm::vec2 Font::MeasureText(const std::string& text, float textHeightPx) const
{
	if (text.empty())
		return {0.f, 0.f};

	float scale = textHeightPx / m_GlyphSize.y;

	float width = 0.f;
	for (int i = 0; i < text.size(); i++)
	{
		char c = text[i];
		int ascii = static_cast<unsigned char>(c);
		width += m_Glyphs[ascii].advance * scale;
	}

	float height = m_GlyphSize.y * scale;

	return {width, height};
}

glm::ivec2 onion::voxel::Font::GetGlyphSize() const
{
	return m_GlyphSize;
}

// -------- OpenGL Buffer Setup --------

void Font::GenerateBuffers()
{
	glGenVertexArrays(1, &m_VAO);
	glGenBuffers(1, &m_VBO);

	glBindVertexArray(m_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

	glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, posX));
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, texX));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
}

void Font::DeleteBuffers()
{
	if (m_VAO)
		glDeleteVertexArrays(1, &m_VAO);

	if (m_VBO)
		glDeleteBuffers(1, &m_VBO);

	m_VAO = 0;
	m_VBO = 0;
}

float GetGlyphAdvance(
	unsigned char* data, int col, int row, int glyphPixelWidth, int glyphPixelHeight, int textureWidth, int nbChannels)
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

	return lastNonEmpty - firstNonEmpty + 2.5;
}

void onion::voxel::Font::InitializeGlyphs()
{
	auto data = m_TextureAtlas.GetData();
	int width = m_TextureAtlas.Width();
	int height = m_TextureAtlas.Height();
	int nrChannels = m_TextureAtlas.Channels();

	int glyphPixelWidth = width / m_AtlasCols;
	int glyphPixelHeight = height / m_AtlasRows;

	for (int i = 0; i < 256; i++)
	{
		int col = i % m_AtlasCols;
		int row = i / m_AtlasCols;

		m_Glyphs[i].width = (float) glyphPixelWidth;
		m_Glyphs[i].height = (float) glyphPixelHeight;

		m_Glyphs[i].advance =
			GetGlyphAdvance(data.get(), col, row, glyphPixelWidth, glyphPixelHeight, width, nrChannels);

		float uvStepX = 1.0f / m_AtlasCols;
		float uvStepY = 1.0f / m_AtlasRows;

		m_Glyphs[i].u0 = col * uvStepX;
		m_Glyphs[i].v0 = row * uvStepY;
		m_Glyphs[i].u1 = m_Glyphs[i].u0 + uvStepX;
		m_Glyphs[i].v1 = m_Glyphs[i].v0 + uvStepY;
	}

	m_GlyphSize = glm::ivec2(m_TextureAtlas.Width() / m_AtlasCols, m_TextureAtlas.Height() / m_AtlasRows);
}
