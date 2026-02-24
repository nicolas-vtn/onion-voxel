#include "Button.hpp"

#include <iostream>

#include "../../../Variables.hpp"

namespace onion::voxel
{

	// -------- Static Data --------

	std::vector<Button::Vertex> Button::s_Vertices = {
		{0.f, 0.f, 0.f, 0.f, 0.f}, {1.f, 0.f, 0.f, 1.f, 0.f}, {1.f, 1.f, 0.f, 1.f, 1.f}, {0.f, 1.f, 0.f, 0.f, 1.f}};

	std::vector<unsigned int> Button::s_Indices = {0, 1, 2, 2, 3, 0};

	Texture Button::s_Texture((GetMinecraftAssetsPath() / "textures/gui/sprites/widget/button.png").string().c_str());
	Texture Button::s_TextureDisabled(
		(GetMinecraftAssetsPath() / "textures/gui/sprites/widget/button_disabled.png").string().c_str());
	Texture Button::s_TextureHighlighted(
		(GetMinecraftAssetsPath() / "textures/gui/sprites/widget/button_highlighted.png").string().c_str());

	// -------- Constructor --------

	Button::Button(const std::string& name)
		: GuiElement(name), m_NineSliceSprite(GetMinecraftAssetsPath() / "textures/gui/sprites/widget/button.png")
	{
	}

	Button::~Button() {}

	// -------- Public API --------

	void Button::Render()
	{
		if (!s_InputsSnapshot)
		{
			std::cerr << "Error: Button::Render() called without a valid InputsSnapshot." << std::endl;
			return;
		}

		bool isCurrentlyHovered = IsHovered();
		bool isClicked = s_InputsSnapshot->Mouse.LeftButtonPressed;

		// ----- Hover Logic -----
		if (isCurrentlyHovered)
		{
			if (!m_WasHovered)
			{
				std::cout << "Button '" << GetName() << "' hovered." << std::endl;
				OnHover.Trigger(*this);
			}
		}
		else
		{
			if (m_WasHovered)
			{
				std::cout << "Button '" << GetName() << "' unhovered." << std::endl;
				OnUnhover.Trigger(*this);
			}
		}

		// ----- Click Logic -----
		if (m_IsEnabled && isCurrentlyHovered && !isClicked && m_WasClicked)
		{
			std::cout << "Button '" << GetName() << "' clicked." << std::endl;
			OnClick.Trigger(*this);
		}

		// ----- Update States ----
		m_WasHovered = isCurrentlyHovered;
		m_WasClicked = isClicked;

		float scaleFactor = 1.0f;

		// ----- Scale Up on Hover -----
		if (m_IsEnabled && m_ScaleUpOnHover && isCurrentlyHovered)
			scaleFactor *= 1.05f;

		// ----- Scale Down on Click -----
		if (m_IsEnabled && isCurrentlyHovered && isClicked)
			scaleFactor *= 0.95f;

		glm::vec2 updatedSize = m_Size * scaleFactor;

		glm::vec2 topLeft = m_Position - updatedSize * 0.5f;

		// ----- Render Button -----
		//s_ShaderSprites.Use();
		//s_ShaderSprites.setVec2("uPos", topLeft.x, topLeft.y);
		//s_ShaderSprites.setVec2("uSize", updatedSize.x, updatedSize.y);
		//s_ShaderSprites.setInt("uTexture", 0);

		//glActiveTexture(GL_TEXTURE0);

		//// Bind correct texture based on state
		//if (!m_IsEnabled)
		//{
		//	s_TextureDisabled.Bind();
		//}
		//else if (isCurrentlyHovered)
		//{
		//	s_TextureHighlighted.Bind();
		//}
		//else
		//{
		//	s_Texture.Bind();
		//}

		//glBindVertexArray(m_VAO);
		//glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(s_Indices.size()), GL_UNSIGNED_INT, 0);
		//glBindVertexArray(0);

		m_NineSliceSprite.SetPosition(m_Position);
		m_NineSliceSprite.SetSize(m_Size);
		m_NineSliceSprite.Render();

		// ----- Render Text -----
		if (!m_Text.empty())
		{
			float textScale = m_Size.y / 19.f;
			textScale *= scaleFactor;

			glm::vec2 textSize = s_TextFont.MeasureText(m_Text, textScale);

			float textX = topLeft.x + (updatedSize.x - textSize.x) * 0.5f;
			float textY = topLeft.y + (updatedSize.y - textSize.y) * 0.5f;

			float shadowOffset = m_Size.y * 0.06f;
			s_TextFont.RenderText(
				m_Text, textX + shadowOffset, textY + shadowOffset, textScale, {0.247f, 0.247f, 0.247f});
			s_TextFont.RenderText(m_Text, textX, textY, textScale, {1, 1, 1});
		}
	}

	void Button::Initialize()
	{
		GenerateBuffers();
		InitBuffers();

		m_NineSliceSprite.Initialize();

		SetInitState(true);
	}

	void Button::Delete()
	{
		DeleteBuffers();
		m_NineSliceSprite.Delete();
		SetDeletedState(true);
	}

	void Button::SetText(const std::string& text)
	{
		m_Text = text;
	}

	std::string Button::GetText() const
	{
		return m_Text;
	}

	void Button::SetSize(const glm::vec2& size)
	{
		m_Size = size;
	}

	glm::vec2 Button::GetSize() const
	{
		return m_Size;
	}

	void Button::SetPosition(double posX, double posY)
	{
		m_Position = {static_cast<float>(posX), static_cast<float>(posY)};
	}

	void Button::SetPosition(const glm::vec2& pos)
	{
		m_Position = pos;
	}

	glm::vec2 Button::GetPosition() const
	{
		return m_Position;
	}

	bool Button::IsEnabled() const
	{
		return m_IsEnabled;
	}

	void Button::SetEnabled(bool enabled)
	{
		m_IsEnabled = enabled;
	}

	void Button::SetScaleUpOnHover(bool scaleUp)
	{
		m_ScaleUpOnHover = scaleUp;
	}

	// -------- Hover Logic --------

	bool Button::IsHovered() const
	{
		if (!s_InputsSnapshot)
		{
			return false;
		}

		int mouseX = s_InputsSnapshot->Mouse.Xpos;
		int mouseY = s_InputsSnapshot->Mouse.Ypos;

		glm::vec2 topLeft = m_Position - (m_Size * 0.5f);

		bool hovered = mouseX >= topLeft.x && mouseX <= topLeft.x + m_Size.x && mouseY >= topLeft.y &&
			mouseY <= topLeft.y + m_Size.y;

		return hovered;
	}

	// -------- OpenGL Buffer Setup --------

	void Button::GenerateBuffers()
	{
		glGenVertexArrays(1, &m_VAO);
		glGenBuffers(1, &m_VBO);
		glGenBuffers(1, &m_EBO);
	}

	void Button::DeleteBuffers()
	{
		glDeleteVertexArrays(1, &m_VAO);
		glDeleteBuffers(1, &m_VBO);
		glDeleteBuffers(1, &m_EBO);

		m_VAO = 0;
		m_VBO = 0;
		m_EBO = 0;
	}

	void Button::InitBuffers()
	{
		glBindVertexArray(m_VAO);

		glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
		glBufferData(GL_ARRAY_BUFFER, s_Vertices.size() * sizeof(Vertex), s_Vertices.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
		glBufferData(
			GL_ELEMENT_ARRAY_BUFFER, s_Indices.size() * sizeof(unsigned int), s_Indices.data(), GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, posX));
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, texX));
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);
	}

} // namespace onion::voxel
