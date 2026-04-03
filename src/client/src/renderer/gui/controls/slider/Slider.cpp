#include "Slider.hpp"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

namespace onion::voxel
{
	Slider::Slider(const std::string& name)
		: GuiElement(name), m_Label(name + "_SliderLabel"),
		  m_NineSliceSprite_SliderBasic(name + "_9Slice_SliderBasic", s_SpritePathFromGui_Slider),
		  m_NineSliceSprite_SliderHighlighted(name + "_9Slice_SliderHighlighted",
											  s_SpritePathFromGui_SliderHighlighted),
		  m_NineSliceSprite_HandleBasic(name + "_9Slice_HandleBasic", s_SpritePathFromGui_HandleBasic),
		  m_NineSliceSprite_HandleHighlighted(name + "_9Slice_HandleHighlighted", s_SpritePathFromGui_HandleHighlighted)
	{
		SubscribeToSpriteEvents();

		m_NineSliceSprite_SliderBasic.SetZOffset(0.5f);
		m_NineSliceSprite_SliderHighlighted.SetZOffset(0.5f);

		m_NineSliceSprite_HandleBasic.SetZOffset(0.6f);
		m_NineSliceSprite_HandleHighlighted.SetZOffset(0.6f);

		m_Label.SetTextAlignment(Font::eTextAlignment::Center);
		m_Label.SetZOffset(0.8f);
		m_Label.SetTextColor(s_TextColor);
		m_Label.SetShadowColor(s_TextShadowColor);
	}

	Slider::~Slider()
	{
		m_EventHandles.clear();
	}

	void Slider::Initialize()
	{
		m_NineSliceSprite_SliderBasic.Initialize();
		m_NineSliceSprite_SliderHighlighted.Initialize();
		m_NineSliceSprite_HandleBasic.Initialize();
		m_NineSliceSprite_HandleHighlighted.Initialize();
		m_Label.Initialize();

		SetInitState(true);
	}
	void Slider::Render()
	{
		if (!s_InputsSnapshot)
		{
			std::cerr << "Error: Slider::Render() called without a valid InputsSnapshot." << std::endl;
			return;
		}

		// DEBUG
		//if (EngineContext::Get().ShowDebugMenus)
		//	RenderImGuiDebug();

		m_NineSliceSprite_SliderBasic.PullEvents();

		// Calculate Value if Sliding
		if (m_IsSliding)
		{
			glm::ivec2 mousePos = {s_InputsSnapshot->Mouse.Xpos, s_InputsSnapshot->Mouse.Ypos};
			m_Value = GetValueFromMousePosition(mousePos);

			if (m_Value != m_PrevValue)
			{
				m_PrevValue = m_Value;
				OnValueChanged.Trigger(*this);

				//std::cout << "Slider Value Changed: " << m_Value << std::endl;
			}
		}

		// Render NineSliceSprites
		bool hovered = m_NineSliceSprite_SliderBasic.IsHovered();
		m_NineSliceSprite_SliderBasic.Render();

		// Calculate Handle Position
		glm::ivec2 centerPos = GetPosition();
		int startX = centerPos.x - m_Size.x / 2 + m_HandleWidth / 2;
		int endX = centerPos.x + m_Size.x / 2 - m_HandleWidth / 2;
		float valueRatio = static_cast<float>(m_Value) / m_MaxValue;
		float slidingWidth = static_cast<float>(endX - startX);
		int handleX = static_cast<int>(startX + valueRatio * slidingWidth);
		const glm::ivec2 handlePos{handleX, centerPos.y};

		m_NineSliceSprite_HandleHighlighted.SetPosition(handlePos);
		m_NineSliceSprite_HandleBasic.SetPosition(handlePos);

		// Render Handle
		if (hovered || m_IsSliding)
		{
			m_NineSliceSprite_HandleHighlighted.Render();
		}
		else
		{
			m_NineSliceSprite_HandleBasic.Render();
		}

		// Render Text
		m_Label.Render();
	}
	void Slider::Delete()
	{
		m_NineSliceSprite_SliderBasic.Delete();
		m_NineSliceSprite_SliderHighlighted.Delete();
		m_NineSliceSprite_HandleBasic.Delete();
		m_NineSliceSprite_HandleHighlighted.Delete();
		m_Label.Delete();

		SetDeletedState(true);
	}

	void Slider::ReloadTextures()
	{
		m_NineSliceSprite_SliderBasic.ReloadTextures();
		m_NineSliceSprite_SliderHighlighted.ReloadTextures();
		m_NineSliceSprite_HandleBasic.ReloadTextures();
		m_NineSliceSprite_HandleHighlighted.ReloadTextures();
		m_Label.ReloadTextures();
	}

	void Slider::SetText(const std::string& text)
	{
		m_Label.SetText(text);
	}

	std::string Slider::GetText() const
	{
		return m_Label.GetText();
	}

	void Slider::SetSize(const glm::ivec2& size)
	{
		m_Size = size;

		// Update Slider Size
		m_NineSliceSprite_SliderBasic.SetSize(size);
		m_NineSliceSprite_SliderHighlighted.SetSize(size);

		// Update Handle Height
		m_HandleWidth = static_cast<int>(std::lround(0.4f * size.y));
		const glm::ivec2 handleSize = {m_HandleWidth, size.y};
		m_NineSliceSprite_HandleBasic.SetSize(handleSize);
		m_NineSliceSprite_HandleHighlighted.SetSize(handleSize);

		// Update Label Size
		m_Label.SetTextHeight(size.y * m_TextHeightRatio);
	}

	glm::ivec2 Slider::GetSize() const
	{
		return m_Size;
	}

	void Slider::SetPosition(const glm::ivec2& pos)
	{
		m_Position = pos;

		// Update Slider Position
		m_NineSliceSprite_SliderBasic.SetPosition(pos);
		m_NineSliceSprite_SliderHighlighted.SetPosition(pos);

		// Update Label Position
		m_Label.SetPosition(pos);
	}

	glm::ivec2 Slider::GetPosition() const
	{
		return m_Position;
	}

	void Slider::SetValue(uint32_t value)
	{
		if (value > m_MaxValue)
			value = m_MaxValue;

		m_Value = value;
	}

	uint32_t Slider::GetValue() const
	{
		return m_Value;
	}

	void Slider::SetMaxValue(uint32_t maxValue)
	{
		if (maxValue == 0)
			maxValue = 1;

		m_MaxValue = maxValue;
	}

	uint32_t Slider::GetMaxValue() const
	{
		return m_MaxValue;
	}

	void Slider::SubscribeToSpriteEvents()
	{
		m_EventHandles.push_back(m_NineSliceSprite_SliderBasic.OnMouseDown.Subscribe(
			[this](const NineSliceSprite& sprite) { Handle_MouseDown(sprite); }));

		m_EventHandles.push_back(m_NineSliceSprite_SliderBasic.OnMouseUp.Subscribe([this](const NineSliceSprite& sprite)
																				   { Handle_MouseUp(sprite); }));

		m_EventHandles.push_back(m_NineSliceSprite_SliderBasic.OnHoverEnter.Subscribe(
			[this](const NineSliceSprite& sprite) { Handle_HoverEnter(sprite); }));

		m_EventHandles.push_back(m_NineSliceSprite_SliderBasic.OnHoverLeave.Subscribe(
			[this](const NineSliceSprite& sprite) { Handle_HoverLeave(sprite); }));
	}

	void Slider::Handle_MouseDown(const NineSliceSprite& sprite)
	{
		(void) sprite; // Unused parameter
		m_IsSliding = true;
	}

	void Slider::Handle_MouseUp(const NineSliceSprite& sprite)
	{
		(void) sprite; // Unused parameter
		m_IsSliding = false;
	}

	void Slider::Handle_HoverEnter(const NineSliceSprite& sprite)
	{
		(void) sprite; // Unused parameter

		const auto& inputs = EngineContext::Get().Inputs;
		inputs->SetCursorStyle(CursorStyle::Hand);
	}

	void Slider::Handle_HoverLeave(const NineSliceSprite& sprite)
	{
		(void) sprite; // Unused parameter

		const auto& inputs = EngineContext::Get().Inputs;
		inputs->SetCursorStyle(CursorStyle::Arrow);
	}

	uint32_t Slider::GetValueFromMousePosition(const glm::ivec2& mousePosition) const
	{
		glm::ivec2 centerPos = GetPosition();
		int startX = centerPos.x - m_Size.x / 2 + m_HandleWidth / 2;
		int endX = centerPos.x + m_Size.x / 2 - m_HandleWidth / 2;

		if (mousePosition.x <= startX)
			return 0;

		if (mousePosition.x >= endX)
			return m_MaxValue;

		float slidingWidth = static_cast<float>(endX - startX);
		float valueRatio = static_cast<float>(mousePosition.x - startX) / slidingWidth;

		return static_cast<uint32_t>(std::round(valueRatio * m_MaxValue));
	}

	void Slider::RenderImGuiDebug()
	{ // Debug ImGui pannel
		ImGui::Begin(("Slider: " + GetName()).c_str());

		ImGui::Text("Hovered: %s", m_NineSliceSprite_SliderBasic.IsHovered() ? "Yes" : "No");
		ImGui::Text("Sliding: %s", m_IsSliding ? "Yes" : "No");

		std::string text = GetText();
		if (ImGui::InputText("Text", &text))
		{
			SetText(text);
		}

		ImGui::Separator();

		uint32_t maxValue = GetMaxValue();
		if (ImGui::SliderInt("Max Value", (int*) &maxValue, 1, 100))
		{
			SetMaxValue(maxValue);
		}

		uint32_t value = GetValue();
		if (ImGui::SliderInt("Value", (int*) &value, 0, static_cast<int>(maxValue)))
		{
			SetValue(value);
		}

		ImGui::Separator();

		ImGui::SliderFloat("Text Height Ratio", &m_TextHeightRatio, 0.3f, 0.4f);
		ImGui::SliderInt("Handle Width", &m_HandleWidth, 1, 100);

		ImGui::Separator();

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
