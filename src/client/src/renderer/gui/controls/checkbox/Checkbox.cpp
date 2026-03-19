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
		SubscribeToSpriteEvents();
	}

	Checkbox::~Checkbox()
	{
		m_EventHandles.clear();
	}

	void Checkbox::Render()
	{
		m_Checkbox_Sprite.PullEvents();

		bool hovered = IsHovered();

		Sprite* spriteToRender = nullptr;
		if (hovered)
		{
			spriteToRender = m_Checked ? &m_CheckboxSelectedHighlighted_Sprite : &m_CheckboxHighlighted_Sprite;
		}
		else
		{
			spriteToRender = m_Checked ? &m_CheckboxSelected_Sprite : &m_Checkbox_Sprite;
		}

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
		if (size == m_Size)
			return;

		m_Size = size;
		m_Checkbox_Sprite.SetSize(size);
		m_CheckboxHighlighted_Sprite.SetSize(size);
		m_CheckboxSelected_Sprite.SetSize(size);
		m_CheckboxSelectedHighlighted_Sprite.SetSize(size);
	}

	glm::vec2 Checkbox::GetSize() const
	{
		return m_Size;
	}

	void Checkbox::SetPosition(const glm::vec2& pos)
	{
		if (pos == m_Position)
			return;

		m_Position = pos;
		m_Checkbox_Sprite.SetPosition(pos);
		m_CheckboxHighlighted_Sprite.SetPosition(pos);
		m_CheckboxSelected_Sprite.SetPosition(pos);
		m_CheckboxSelectedHighlighted_Sprite.SetPosition(pos);
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

	void Checkbox::SetVisibility(const Visibility& visibility)
	{
		GuiElement::SetVisibility(visibility);

		m_Checkbox_Sprite.SetVisibility(visibility);
		m_CheckboxHighlighted_Sprite.SetVisibility(visibility);
		m_CheckboxSelected_Sprite.SetVisibility(visibility);
		m_CheckboxSelectedHighlighted_Sprite.SetVisibility(visibility);
	}

	void Checkbox::SubscribeToSpriteEvents()
	{
		m_EventHandles.push_back(
			m_Checkbox_Sprite.OnClick.Subscribe([this](const Sprite& sprite) { Handle_Click(sprite); }));

		m_EventHandles.push_back(
			m_Checkbox_Sprite.OnHoverEnter.Subscribe([this](const Sprite& sprite) { Handle_HoverEnter(sprite); }));

		m_EventHandles.push_back(
			m_Checkbox_Sprite.OnHoverLeave.Subscribe([this](const Sprite& sprite) { Handle_HoverLeave(sprite); }));
	}

	void Checkbox::Handle_Click(const Sprite& sprite)
	{
		(void) sprite; // Unused parameter

		m_Checked = !m_Checked;
		OnCheckedChanged.Trigger(*this);
	}

	void Checkbox::Handle_HoverEnter(const Sprite& sprite)
	{
		(void) sprite; // Unused parameter

		auto& inputs = EngineContext::Get().Inputs;
		inputs->SetCursorStyle(CursorStyle::Hand);
	}

	void Checkbox::Handle_HoverLeave(const Sprite& sprite)
	{
		(void) sprite; // Unused parameter

		auto& inputs = EngineContext::Get().Inputs;
		inputs->SetCursorStyle(CursorStyle::Arrow);
	}

	bool Checkbox::IsHovered() const
	{
		return m_Checkbox_Sprite.IsHovered();
	}

} // namespace onion::voxel
