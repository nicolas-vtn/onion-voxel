#include "Checkbox.hpp"

namespace onion::voxel
{
	Checkbox::Checkbox(const std::string& name)
		: GuiElement(name), m_Checkbox_Sprite("Checkbox_Sprite", s_SpritePathFromGui, Sprite::eOrigin::ResourcePack),
		  m_CheckboxHighlighted_Sprite(
			  "CheckboxHighlighted_Sprite", s_SpritePathFromGui_Highlighted, Sprite::eOrigin::ResourcePack),
		  m_CheckboxSelected_Sprite(
			  "CheckboxSelected_Sprite", s_SpritePathFromGui_Selected, Sprite::eOrigin::ResourcePack),
		  m_CheckboxSelectedHighlighted_Sprite("CheckboxSelectedHighlighted_Sprite",
											   s_SpritePathFromGui_SelectedHighlighted,
											   Sprite::eOrigin::ResourcePack)
	{
	}

	Checkbox::~Checkbox() {}

	void Checkbox::Render()
	{
		bool hovered = IsHovered();

		// Click detection
		if (s_InputsSnapshot)
		{
			bool isMouseDown = s_InputsSnapshot->Mouse.LeftButtonPressed;
			if (hovered && isMouseDown && !m_WasMouseDown)
			{
				m_Checked = !m_Checked;
				OnCheckedChanged.Trigger(*this);
			}
			m_WasMouseDown = isMouseDown;
		}

		Sprite* spriteToRender = nullptr;
		if (hovered)
		{
			spriteToRender = m_Checked ? &m_CheckboxSelectedHighlighted_Sprite : &m_CheckboxHighlighted_Sprite;
		}
		else
		{
			spriteToRender = m_Checked ? &m_CheckboxSelected_Sprite : &m_Checkbox_Sprite;
		}

		spriteToRender->SetPosition(m_Position);
		spriteToRender->SetSize(m_Size);
		spriteToRender->Render();
	}

	void Checkbox::Initialize()
	{
		m_Checkbox_Sprite.Initialize();
		m_CheckboxHighlighted_Sprite.Initialize();
		m_CheckboxSelected_Sprite.Initialize();
		m_CheckboxSelectedHighlighted_Sprite.Initialize();

		SetInitState(true);
	}

	void Checkbox::Delete()
	{
		m_Checkbox_Sprite.Delete();
		m_CheckboxHighlighted_Sprite.Delete();
		m_CheckboxSelected_Sprite.Delete();
		m_CheckboxSelectedHighlighted_Sprite.Delete();

		SetDeletedState(true);
	}

	void Checkbox::ReloadTextures()
	{
		m_Checkbox_Sprite.ReloadTextures();
		m_CheckboxHighlighted_Sprite.ReloadTextures();
		m_CheckboxSelected_Sprite.ReloadTextures();
		m_CheckboxSelectedHighlighted_Sprite.ReloadTextures();
	}

	void Checkbox::SetSize(const glm::vec2& size)
	{
		m_Size = size;
	}

	glm::vec2 Checkbox::GetSize() const
	{
		return m_Size;
	}

	void Checkbox::SetPosition(const glm::vec2& pos)
	{
		m_Position = pos;
	}

	glm::vec2 Checkbox::GetPosition() const
	{
		return m_Position;
	}

	void Checkbox::SetChecked(bool checked)
	{
		m_Checked = checked;
	}

	bool Checkbox::IsChecked() const
	{
		return m_Checked;
	}

	bool Checkbox::IsHovered() const
	{
		if (s_InputsSnapshot == nullptr)
		{
			return false;
		}

		glm::vec2 mousePos{s_InputsSnapshot->Mouse.Xpos, s_InputsSnapshot->Mouse.Ypos};
		glm::vec2 topLeft = m_Position - m_Size * 0.5f;
		glm::vec2 bottomRight = m_Position + m_Size * 0.5f;
		return mousePos.x >= topLeft.x && mousePos.x <= bottomRight.x && mousePos.y >= topLeft.y &&
			mousePos.y <= bottomRight.y;
	}

} // namespace onion::voxel
