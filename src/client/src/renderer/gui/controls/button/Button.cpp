#include "Button.hpp"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <iostream>

namespace onion::voxel
{
	// -------- Constructor --------

	Button::Button(const std::string& name)
		: GuiElement(name), m_NineSliceSprite_Basic(name + "_9Slice_Basic", s_SpritePathFromGui),
		  m_NineSliceSprite_Disabled(name + "_9Slice_Disabled", s_SpritePathFromGui_Disabled),
		  m_NineSliceSprite_Highlighted(name + "_9Slice_Highlighted", s_SpritePathFromGui_Highlighted),
		  m_Label(name + "_Label")
	{
		SubscribeToSpriteEvents();

		m_Label.SetTextAlignment(Font::eTextAlignment::Center);
		m_Label.SetZOffset(0.8f);
	}

	Button::~Button()
	{
		m_EventHandles.clear();
	}

	// -------- Public API --------

	void Button::Render()
	{
		if (!s_InputsSnapshot)
		{
			std::cerr << "Error: Button::Render() called without a valid InputsSnapshot." << std::endl;
			return;
		}

		// DEBUG
		//if (EngineContext::Get().ShowDebugMenus)
		//	RenderImGuiDebug();

		// Pulls events
		m_NineSliceSprite_Basic.PullEvents();

		// Render the appropriate sprite based on the button's state
		if (m_IsEnabled)
		{
			bool isCurrentlyHovered = m_NineSliceSprite_Basic.IsHovered();
			if (isCurrentlyHovered)
				m_NineSliceSprite_Highlighted.Render();
			else
				m_NineSliceSprite_Basic.Render();
		}
		else
		{
			m_NineSliceSprite_Disabled.Render();
		}

		// ----- Render Label -----

		glm::ivec2 size = GetSize();
		if (m_IsPressed && m_IsEnabled)
		{
			size = glm::ivec2(glm::vec2(size) * m_ScaleFactorOnClick);
		}
		float textHeight = size.y * 0.4f;

		glm::ivec2 textCenter = GetPosition();

		constexpr Font::eColor disabledTextColor = Font::eColor::Gray;
		Font::eColor textColor = m_IsEnabled ? s_ColorMainText : disabledTextColor;

		m_Label.SetPosition(textCenter);
		m_Label.SetTextColor(textColor);
		m_Label.SetTextHeight(textHeight);
		m_Label.EnableShadow(m_IsEnabled);

		m_Label.Render();
	}

	void Button::Initialize()
	{
		m_NineSliceSprite_Basic.Initialize();
		m_NineSliceSprite_Disabled.Initialize();
		m_NineSliceSprite_Highlighted.Initialize();
		m_Label.Initialize();

		SetInitState(true);
	}

	void Button::Delete()
	{
		m_NineSliceSprite_Basic.Delete();
		m_NineSliceSprite_Disabled.Delete();
		m_NineSliceSprite_Highlighted.Delete();
		m_Label.Delete();

		SetDeletedState(true);
	}

	void Button::ReloadTextures()
	{
		m_NineSliceSprite_Basic.ReloadTextures();
		m_NineSliceSprite_Disabled.ReloadTextures();
		m_NineSliceSprite_Highlighted.ReloadTextures();
	}

	void Button::SetText(const std::string& text)
	{
		if (m_Label.GetText() == text)
			return;

		m_Label.SetText(text);
	}

	std::string Button::GetText() const
	{
		return m_Label.GetText();
	}

	void Button::SetSize(const glm::ivec2& size)
	{
		if (size == m_Size)
			return;

		m_Size = size;
		m_NineSliceSprite_Basic.SetSize(size);
		m_NineSliceSprite_Disabled.SetSize(size);
		m_NineSliceSprite_Highlighted.SetSize(size);
	}

	glm::ivec2 Button::GetSize() const
	{
		return m_Size;
	}

	void Button::SetPosition(const glm::ivec2& pos)
	{
		if (pos == m_Position)
			return;

		m_Position = pos;
		m_NineSliceSprite_Basic.SetPosition(pos);
		m_NineSliceSprite_Disabled.SetPosition(pos);
		m_NineSliceSprite_Highlighted.SetPosition(pos);
	}

	glm::ivec2 Button::GetPosition() const
	{
		return m_Position;
	}

	bool Button::IsHovered() const
	{
		return m_NineSliceSprite_Basic.IsHovered();
	}

	void Button::SetVisibility(const Visibility& visibility)
	{
		GuiElement::SetVisibility(visibility);

		m_NineSliceSprite_Basic.SetVisibility(visibility);
		m_NineSliceSprite_Disabled.SetVisibility(visibility);
		m_NineSliceSprite_Highlighted.SetVisibility(visibility);
		m_Label.SetVisibility(visibility);
	}

	bool Button::IsEnabled() const
	{
		return m_IsEnabled;
	}

	void Button::SetEnabled(bool enabled)
	{
		m_IsEnabled = enabled;
	}

	void Button::SubscribeToSpriteEvents()
	{

		// We subscribe to only ONE sprite, since they all share the same position and size, so their hovered state will always be the same.

		m_EventHandles.push_back(m_NineSliceSprite_Basic.EvtMouseDown.Subscribe([this](const NineSliceSprite& sprite)
																				{ HandleMouseDown(sprite); }));

		m_EventHandles.push_back(m_NineSliceSprite_Basic.EvtMouseUp.Subscribe([this](const NineSliceSprite& sprite)
																			  { HandleMouseUp(sprite); }));

		m_EventHandles.push_back(m_NineSliceSprite_Basic.EvtClick.Subscribe([this](const NineSliceSprite& sprite)
																			{ HandleSpriteClick(sprite); }));

		m_EventHandles.push_back(m_NineSliceSprite_Basic.EvtHoverEnter.Subscribe([this](const NineSliceSprite& sprite)
																				 { HandleSpriteHoverEnter(sprite); }));

		m_EventHandles.push_back(m_NineSliceSprite_Basic.EvtHoverLeave.Subscribe([this](const NineSliceSprite& sprite)
																				 { HandleSpriteHoverLeave(sprite); }));
	}

	void Button::HandleMouseDown(const NineSliceSprite& sprite)
	{
		(void) sprite; // Unused parameter
		m_IsPressed = true;
		glm::ivec2 updatedSize = glm::ivec2(glm::vec2(m_Size) * m_ScaleFactorOnClick);

		if (m_IsEnabled)
		{
			m_NineSliceSprite_Basic.SetSize(updatedSize);
			m_NineSliceSprite_Disabled.SetSize(updatedSize);
			m_NineSliceSprite_Highlighted.SetSize(updatedSize);
		}
	}

	void Button::HandleMouseUp(const NineSliceSprite& sprite)
	{
		(void) sprite; // Unused parameter
		m_IsPressed = false;
		m_NineSliceSprite_Basic.SetSize(m_Size);
		m_NineSliceSprite_Disabled.SetSize(m_Size);
		m_NineSliceSprite_Highlighted.SetSize(m_Size);
	}

	void Button::HandleSpriteClick(const NineSliceSprite& sprite)
	{
		(void) sprite; // Unused parameter
		if (m_IsEnabled)
			EvtClick.Trigger(*this);
	}

	void Button::HandleSpriteHoverEnter(const NineSliceSprite& sprite)
	{
		(void) sprite; // Unused parameter
		if (m_IsEnabled)
		{
			GuiElement::EvtRequestCursorStyleChange.Trigger(CursorStyle::Hand);
		}
		EvtHoverEnter.Trigger(*this);
	}

	void Button::HandleSpriteHoverLeave(const NineSliceSprite& sprite)
	{
		(void) sprite; // Unused parameter
		if (m_IsEnabled)
		{
			GuiElement::EvtRequestCursorStyleChange.Trigger(CursorStyle::Arrow);
		}
		EvtHoverLeave.Trigger(*this);
	}

	void Button::RenderImGuiDebug()
	{
		// Debug ImGui pannel
		ImGui::Begin(("Button: " + GetName()).c_str());

		ImGui::Text("Hovered: %s", m_NineSliceSprite_Basic.IsHovered() ? "Yes" : "No");

		std::string text = GetText();
		if (ImGui::InputText("Text", &text))
		{
			SetText(text);
		}

		bool isEnabled = IsEnabled();
		if (ImGui::Checkbox("Enabled", &isEnabled))
		{
			SetEnabled(isEnabled);
		}

		ImGui::SliderFloat("Scale On Click", &m_ScaleFactorOnClick, 0.2f, 1.f);

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

		int guiScale = GuiElement::GetGuiScale();
		ImGui::SliderInt("GuiScale", &guiScale, 1, 16);
		if (guiScale != GuiElement::GetGuiScale())
		{
			GuiElement::SetGuiScale(guiScale);
		}

		ImGui::End();
	}

} // namespace onion::voxel
