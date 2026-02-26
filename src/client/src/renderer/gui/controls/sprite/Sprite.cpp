#include "Sprite.hpp"

using namespace onion::voxel;

// -------- Static Data --------

std::vector<Sprite::Vertex> Sprite::s_Vertices = {
	{0.f, 0.f, 0.f, 0.f, 0.f}, {1.f, 0.f, 0.f, 1.f, 0.f}, {1.f, 1.f, 0.f, 1.f, 1.f}, {0.f, 1.f, 0.f, 0.f, 1.f}};

std::vector<unsigned int> Sprite::s_Indices = {0, 1, 2, 2, 3, 0};

Sprite::Sprite(const std::string& name, const std::string& spritePath) : GuiElement(name), m_Texture(spritePath) {}

Sprite::~Sprite() {}

void Sprite::Render()
{
	glm::vec2 topLeft = m_Position - m_Size * 0.5f;

	// ----- Render Sprite -----
	m_Texture.Bind();

	s_ShaderSprites.Use();
	s_ShaderSprites.setVec2("uPos", topLeft.x, topLeft.y);
	s_ShaderSprites.setVec2("uSize", m_Size.x, m_Size.y);
	s_ShaderSprites.setInt("uTexture", 0);

	glActiveTexture(GL_TEXTURE0);

	glBindVertexArray(m_VAO);
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(s_Indices.size()), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void Sprite::Initialize()
{
	GenerateBuffers();
	InitBuffers();

	SetInitState(true);
}

void Sprite::Delete()
{
	m_Texture.Delete();
	DeleteBuffers();
	SetDeletedState(true);
}

void Sprite::SetSize(const glm::vec2& size)
{
	m_Size = size;
}

glm::vec2 Sprite::GetSize() const
{
	return m_Size;
}

void Sprite::SetPosition(const glm::vec2& pos)
{
	m_Position = pos;
}

glm::vec2 Sprite::GetPosition() const
{
	return m_Position;
}

int Sprite::GetTextureWidth() const
{
	return m_Texture.Width();
}

int Sprite::GetTextureHeight() const
{
	return m_Texture.Height();
}

void Sprite::GenerateBuffers()
{
	glGenVertexArrays(1, &m_VAO);
	glGenBuffers(1, &m_VBO);
	glGenBuffers(1, &m_EBO);
}

void Sprite::InitBuffers()
{
	glBindVertexArray(m_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, s_Vertices.size() * sizeof(Vertex), s_Vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, s_Indices.size() * sizeof(unsigned int), s_Indices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, posX));
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, texX));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
}

void Sprite::DeleteBuffers()
{
	glDeleteVertexArrays(1, &m_VAO);
	glDeleteBuffers(1, &m_VBO);
	glDeleteBuffers(1, &m_EBO);

	m_VAO = 0;
	m_VBO = 0;
	m_EBO = 0;
}
