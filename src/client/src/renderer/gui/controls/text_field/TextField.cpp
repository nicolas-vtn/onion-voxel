#include "TextField.hpp"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

namespace onion::voxel
{
	TextField::TextField(const std::string& name)
		: GuiElement(name), m_Label(name + "_TextFieldLabel"),
		  m_NineSliceSprite_TextField(name + "_9Slice_TextField", s_SpritePathFromGui),
		  m_NineSliceSprite_TextFieldHighlighted(name + "_9Slice_TextField_Highlighted",
												 s_SpritePathFromGui_Highlighted)
	{
		SubscribeToSpriteEvents();

		m_Label.SetTextAlignment(Font::eTextAlignment::Left);
		m_Label.SetZOffset(0.8f);
	}

	TextField::~TextField() {}

	void TextField::Initialize()
	{
		m_NineSliceSprite_TextField.Initialize();
		m_NineSliceSprite_TextFieldHighlighted.Initialize();
		m_Label.Initialize();

		SetInitState(true);
	}

	void TextField::Render()
	{
		if (!s_InputsSnapshot)
		{
			std::cerr << "Error: TextField::Render() called without a valid InputsSnapshot." << std::endl;
			return;
		}

		// DEBUG
		RenderImGuiDebug();

		// Pulls events
		m_NineSliceSprite_TextField.PullEvents();
		bool isCurrentlyHovered = m_NineSliceSprite_TextField.IsHovered();

		if (m_IsActive)
			m_NineSliceSprite_TextFieldHighlighted.Render();
		else
			m_NineSliceSprite_TextField.Render();

		// ----- Render Label -----
		glm::ivec2 size = GetSize();
		glm::ivec2 pos = GetPosition();

		glm::ivec2 textPos = pos - glm::ivec2(size.x / 2.1f, 0);

		if (m_Text.empty() && !m_IsActive)
		{
			// PLACEHOLDER TEXT

			m_Label.SetText(m_PlaceholderText);
			m_Label.SetTextColor(s_PlaceholderTextColor);
		}
		else
		{
			// NORMAL TEXT

			m_Label.SetText(m_Text);
			m_Label.SetTextColor(s_TextColor);
		}

		m_Label.SetPosition(textPos);
		m_Label.SetTextHeight(size.y * m_TextScaleFactor);
		m_Label.Render();
	}

	void TextField::Delete()
	{
		m_NineSliceSprite_TextField.Delete();
		m_NineSliceSprite_TextFieldHighlighted.Delete();
		m_Label.Delete();

		SetDeletedState(true);
	}

	void TextField::ReloadTextures()
	{
		m_NineSliceSprite_TextField.ReloadTextures();
		m_NineSliceSprite_TextFieldHighlighted.ReloadTextures();
	}

	void TextField::SetText(const std::string& text)
	{
		m_Text = text;
	}

	std::string TextField::GetText() const
	{
		return m_Text;
	}

	void TextField::SetPlaceholderText(const std::string& placeholderText)
	{
		m_PlaceholderText = placeholderText;
	}

	std::string TextField::GetPlaceholderText() const
	{
		return m_PlaceholderText;
	}

	void TextField::SetSize(const glm::ivec2& size)
	{
		if (size == m_Size)
			return;

		m_Size = size;
		m_NineSliceSprite_TextField.SetSize(size);
		m_NineSliceSprite_TextFieldHighlighted.SetSize(size);
	}

	glm::ivec2 TextField::GetSize() const
	{
		return m_Size;
	}

	void TextField::SetPosition(const glm::ivec2& pos)
	{
		if (pos == m_Position)
			return;

		m_Position = pos;
		m_NineSliceSprite_TextField.SetPosition(pos);
		m_NineSliceSprite_TextFieldHighlighted.SetPosition(pos);
	}

	glm::ivec2 TextField::GetPosition() const
	{
		return m_Position;
	}

	bool TextField::IsReadOnly() const
	{
		return m_ReadOnly;
	}

	void TextField::SetReadOnly(bool readOnly)
	{
		m_ReadOnly = readOnly;
	}

	void TextField::SubscribeToSpriteEvents()
	{
		m_HandleMouseDown = m_NineSliceSprite_TextField.OnMouseDown.Subscribe([this](const NineSliceSprite& sprite)
																			  { HandleMouseDown(sprite); });

		m_HandleMouseUp = m_NineSliceSprite_TextField.OnMouseUp.Subscribe([this](const NineSliceSprite& sprite)
																		  { HandleMouseUp(sprite); });

		m_HandleSpriteClick = m_NineSliceSprite_TextField.OnClick.Subscribe([this](const NineSliceSprite& sprite)
																			{ HandleSpriteClick(sprite); });

		m_HandleSpriteHoverEnter = m_NineSliceSprite_TextField.OnHoverEnter.Subscribe(
			[this](const NineSliceSprite& sprite) { HandleSpriteHoverEnter(sprite); });

		m_HandleSpriteHoverLeave = m_NineSliceSprite_TextField.OnHoverLeave.Subscribe(
			[this](const NineSliceSprite& sprite) { HandleSpriteHoverLeave(sprite); });
	}

	void TextField::HandleMouseDown(const NineSliceSprite& sprite) {}

	void TextField::HandleMouseUp(const NineSliceSprite& sprite) {}

	void TextField::HandleSpriteClick(const NineSliceSprite& sprite) {}

	void TextField::HandleSpriteHoverEnter(const NineSliceSprite& sprite)
	{
		GuiElement::RequestCursorStyleChange.Trigger(CursorStyle::IBeam);
	}

	void TextField::HandleSpriteHoverLeave(const NineSliceSprite& sprite)
	{
		GuiElement::RequestCursorStyleChange.Trigger(CursorStyle::Arrow);
	}

	void TextField::RenderImGuiDebug()
	{
		// Debug ImGui pannel
		ImGui::Begin(("TextField: " + GetName()).c_str());

		ImGui::Text("Hovered: %s", m_NineSliceSprite_TextField.IsHovered() ? "Yes" : "No");

		std::string text = GetText();
		if (ImGui::InputText("Text", &text))
		{
			SetText(text);
		}

		std::string placeholderText = GetPlaceholderText();
		if (ImGui::InputText("Placeholder Text", &placeholderText))
		{
			SetPlaceholderText(placeholderText);
		}

		ImGui::SliderFloat("Text Scale Factor", &m_TextScaleFactor, 0.1f, 1.f);

		bool isReadOnly = IsReadOnly();
		if (ImGui::Checkbox("Read Only", &isReadOnly))
		{
			SetReadOnly(isReadOnly);
		}

		glm::ivec2 position = GetPosition();
		if (ImGui::SliderInt2("Position", &position.x, 0, s_ScreenWidth))
		{
			SetPosition(position);
		}

		glm::ivec2 size = GetSize();
		if (ImGui::SliderInt2("Size", &size.x, 20, 1500))
		{
			SetSize(size);
		}

		ImGui::End();
	}

} // namespace onion::voxel
