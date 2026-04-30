#include "Sprite.hpp"

using namespace onion::voxel;

// -------- Static Data --------

std::vector<Sprite::Vertex> Sprite::s_Vertices = {
	{0.f, 0.f, 0.f, 0.f}, {1.f, 0.f, 1.f, 0.f}, {1.f, 1.f, 1.f, 1.f}, {0.f, 1.f, 0.f, 1.f}};

std::vector<unsigned int> Sprite::s_Indices = {0, 1, 2, 2, 3, 0};

Sprite::Sprite(const std::string& name, const std::filesystem::path& spritePath, eOrigin origin)
	: GuiElement(name), m_Origin(origin), m_SpritePath(spritePath)
{
}

Sprite::Sprite(const std::string& name, Texture texture) : GuiElement(name), m_Texture(std::move(texture))
{
	m_UnreloadableTexture = true;
}

Sprite::~Sprite() {}

void Sprite::Render()
{
	glm::vec2 topLeft = m_Position - m_Size * 0.5f;

	bool hasCissors = !(m_CissorsTopLeft == glm::ivec2(0, 0) && m_CissorsBottomRight == glm::ivec2(0, 0));

	if (hasCissors)
	{
		StartCissors();
	}

	// ----- Setup state for sprites -----
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// ----- Render Sprite -----
	m_Texture.Bind();

	s_ShaderSprites.Use();
	s_ShaderSprites.setVec2("uPos", topLeft.x, topLeft.y);
	s_ShaderSprites.setVec2("uSize", m_Size.x, m_Size.y);
	s_ShaderSprites.setFloat("uZOffset", m_Zoffset);
	s_ShaderSprites.setInt("uTexture", 0);

	glActiveTexture(GL_TEXTURE0);

	glBindVertexArray(m_VAO);
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(s_Indices.size()), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	// ----- Reset state -----
	glDepthMask(GL_TRUE);

	if (hasCissors)
	{
		EndCissors();
	}
}

void Sprite::Initialize()
{
	GenerateBuffers();
	InitBuffers();
	ReloadTextures();

	SetInitState(true);
}

void Sprite::Delete()
{
	m_Texture.Delete();
	DeleteBuffers();
	SetDeletedState(true);
}

void onion::voxel::Sprite::ReloadTextures()
{
	if (m_UnreloadableTexture)
		return;

	// Delete previous texture
	m_Texture.Delete();

	// Load new texture data from the appropriate source based on the origin
	std::vector<unsigned char> data;
	if (m_Origin == eOrigin::Asset)
	{
		data = EngineContext::Get().Assets->GetFileBinary(m_SpritePath);
	}
	else
	{
		data = EngineContext::Get().Assets->GetResourcePackFileBinary(m_SpritePath);
	}

	m_Texture = Texture(m_SpritePath.string(), data);
}

void onion::voxel::Sprite::PullEvents()
{
	// ----- Hover Events -----
	bool isHovered = IsHovered();
	if (isHovered && !m_WasHovered)
	{
		EvtHoverEnter.Trigger(*this);
	}
	else if (!isHovered && m_WasHovered)
	{
		EvtHoverLeave.Trigger(*this);
	}
	m_WasHovered = isHovered;

	// ----- Click Events -----
	bool isMouseDown = s_InputsSnapshot->Mouse.LeftButtonPressed;
	if (isHovered && isMouseDown && !m_WasMouseDown)
	{
		EvtClick.Trigger(*this);
	}
	m_WasMouseDown = isMouseDown;
}

bool onion::voxel::Sprite::IsHovered() const
{
	if (!s_InputsSnapshot)
	{
		return false;
	}

	Visibility visibility = GetVisibility();
	if (!visibility.IsVisible)
	{
		return false;
	}

	int mouseX = (int) std::lround(s_InputsSnapshot->Mouse.Xpos);
	int mouseY = (int) std::lround(s_InputsSnapshot->Mouse.Ypos);

	glm::vec2 topLeft;
	glm::vec2 bottomRight;

	if (visibility.IsFullyVisible)
	{
		topLeft = glm::vec2(m_Position) - glm::vec2(m_Size) * 0.5f;
		bottomRight = glm::vec2(m_Position) + glm::vec2(m_Size) * 0.5f;
	}
	else
	{
		topLeft = glm::vec2(visibility.VisibleAreaTopLeftCorner);
		bottomRight = glm::vec2(visibility.VisibleAreaBottomRightCorner);
	}

	bool hovered = mouseX >= topLeft.x && mouseX <= bottomRight.x && mouseY >= topLeft.y && mouseY <= bottomRight.y;

	return hovered;
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

void onion::voxel::Sprite::SetZOffset(float zOffset)
{
	m_Zoffset = zOffset;
}

float onion::voxel::Sprite::GetZOffset() const
{
	return m_Zoffset;
}

void onion::voxel::Sprite::SetOrigin(eOrigin origin)
{
	m_Origin = origin;
}

onion::voxel::Sprite::eOrigin onion::voxel::Sprite::GetOrigin() const
{
	return m_Origin;
}

void onion::voxel::Sprite::SetCissors(const glm::ivec2& topLeft, const glm::ivec2& bottomRight)
{
	m_CissorsTopLeft = topLeft;
	m_CissorsBottomRight = bottomRight;
}

void onion::voxel::Sprite::SwapTexture(Texture newTexture)
{
	m_Texture.Delete();
	m_Texture = std::move(newTexture);
}

void onion::voxel::Sprite::StartCissors() const
{
	glEnable(GL_SCISSOR_TEST);

	// Convert from top-left origin to bottom-left origin and flip Y axis
	int scissorX = m_CissorsTopLeft.x;
	int scissorY = s_ScreenHeight - m_CissorsBottomRight.y;
	int scissorWidth = m_CissorsBottomRight.x - m_CissorsTopLeft.x;
	int scissorHeight = m_CissorsBottomRight.y - m_CissorsTopLeft.y;

	glScissor(scissorX, scissorY, scissorWidth, scissorHeight);
}

void onion::voxel::Sprite::EndCissors() const
{
	glDisable(GL_SCISSOR_TEST);
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

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, posX));
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
