#include "Button.hpp"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <iostream>

#include "../../../Variables.hpp"

namespace onion::voxel
{
	// -------- Constructor --------

	Button::Button(const std::string& name)
		: GuiElement(name), m_NineSliceSprite_Basic(name + "_9Slice_Basic", GetSpritePath_Basic()),
		  m_NineSliceSprite_Disabled(name + "_9Slice_Disabled", GetSpritePath_Disabled()),
		  m_NineSliceSprite_Highlighted(name + "_9Slice_Highlighted", GetSpritePath_Highlighted())
	{
		SubscribeToSpriteEvents();
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

		// DEBUG
		RenderImGuiDebug();

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

		// ----- Render Text -----
		if (!m_Text.empty())
		{
			glm::ivec2 size = GetSize();
			if (m_IsPressed && m_IsEnabled)
			{
				size = glm::ivec2(glm::vec2(size) * m_ScaleFactorOnClick);
			}

			glm::vec2 topLeft_f = glm::vec2(GetPosition()) - glm::vec2(size) * 0.5f;
			glm::ivec2 topLeft = glm::ivec2(topLeft_f);

			float textScale = size.y / 19.f;

			glm::vec2 textSize = s_TextFont.MeasureText(m_Text, textScale);

			float textX = topLeft.x + (size.x - textSize.x) * 0.5f;
			float textY = topLeft.y + (size.y - textSize.y) * 0.5f;

			textX = std::ceil(textX);
			textY = std::ceil(textY);

			// Render shadow
			if (m_IsEnabled)
			{
				float shadowOffset = size.y * 0.06f;
				glm::vec3 shadowColor = {0.246f, 0.246f, 0.246f};
				s_TextFont.RenderText(m_Text, textX + shadowOffset, textY + shadowOffset, textScale, shadowColor);
			}

			// Render main text
			glm::vec3 textColor = {1.f, 1.f, 1.f};
			if (!m_IsEnabled)
			{
				textColor = {0.625f, 0.625f, 0.625f};
			}
			s_TextFont.RenderText(m_Text, textX, textY, textScale, textColor);
		}
	}

	void Button::Initialize()
	{
		m_NineSliceSprite_Basic.Initialize();
		m_NineSliceSprite_Disabled.Initialize();
		m_NineSliceSprite_Highlighted.Initialize();

		SetInitState(true);
	}

	void Button::Delete()
	{
		m_NineSliceSprite_Basic.Delete();
		m_NineSliceSprite_Disabled.Delete();
		m_NineSliceSprite_Highlighted.Delete();

		SetDeletedState(true);
	}

	void Button::SetText(const std::string& text)
	{
		if (text == m_Text)
			return;

		m_Text = text;
	}

	std::string Button::GetText() const
	{
		return m_Text;
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

		m_HandleMouseDown = m_NineSliceSprite_Basic.OnMouseDown.Subscribe([this](const NineSliceSprite& sprite)
																		  { HandleMouseDown(sprite); });

		m_HandleMouseUp = m_NineSliceSprite_Basic.OnMouseUp.Subscribe([this](const NineSliceSprite& sprite)
																	  { HandleMouseUp(sprite); });

		m_HandleSpriteClick = m_NineSliceSprite_Basic.OnClick.Subscribe([this](const NineSliceSprite& sprite)
																		{ HandleSpriteClick(sprite); });

		m_HandleSpriteHoverEnter = m_NineSliceSprite_Basic.OnHoverEnter.Subscribe([this](const NineSliceSprite& sprite)
																				  { HandleSpriteHoverEnter(sprite); });

		m_HandleSpriteHoverLeave = m_NineSliceSprite_Basic.OnHoverLeave.Subscribe([this](const NineSliceSprite& sprite)
																				  { HandleSpriteHoverLeave(sprite); });
	}

	void Button::HandleMouseDown(const NineSliceSprite& sprite)
	{
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
		m_IsPressed = false;
		m_NineSliceSprite_Basic.SetSize(m_Size);
		m_NineSliceSprite_Disabled.SetSize(m_Size);
		m_NineSliceSprite_Highlighted.SetSize(m_Size);
	}

	void Button::HandleSpriteClick(const NineSliceSprite& sprite)
	{
		if (m_IsEnabled)
			OnClick.Trigger(*this);
	}

	void Button::HandleSpriteHoverEnter(const NineSliceSprite& sprite)
	{
		OnHoverEnter.Trigger(*this);
	}

	void Button::HandleSpriteHoverLeave(const NineSliceSprite& sprite)
	{
		OnHoverLeave.Trigger(*this);
	}

	std::filesystem::path Button::GetSpritePath_Basic()
	{
		return GetMinecraftAssetsPath() / "textures" / "gui" / "sprites" / "widget" / "button.png";
	}

	std::filesystem::path Button::GetSpritePath_Disabled()
	{
		return GetMinecraftAssetsPath() / "textures" / "gui" / "sprites" / "widget" / "button_disabled.png";
	}

	std::filesystem::path Button::GetSpritePath_Highlighted()
	{
		return GetMinecraftAssetsPath() / "textures" / "gui" / "sprites" / "widget" / "button_highlighted.png";
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

		ImGui::SliderInt("GuiScale", &GuiElement::s_GUI_SCALE, 1, 16);

		ImGui::End();
	}

} // namespace onion::voxel
