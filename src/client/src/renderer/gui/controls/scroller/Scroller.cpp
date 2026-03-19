#include "Scroller.hpp"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <renderer/debug_draws/DebugDraws.hpp>

namespace onion::voxel
{
	Scroller::Scroller(const std::string& name)
		: GuiElement(name), m_NineSliceSprite_Scroller("DemoScrollingPanelScroller", s_SpritePathFromGui_Scroller),
		  m_NineSliceSprite_ScrollerBackground("DemoScrollingPanelScrollerBackground",
											   s_SpritePathFromGui_ScrollerBackground)
	{
		SubscribeToSpriteEvents();

		m_NineSliceSprite_Scroller.SetZOffset(0.5f);
		m_NineSliceSprite_ScrollerBackground.SetZOffset(0.4f);
	}

	Scroller::~Scroller() {}

	void Scroller::Initialize()
	{
		m_NineSliceSprite_Scroller.Initialize();
		m_NineSliceSprite_ScrollerBackground.Initialize();

		SetInitState(true);
	}

	void Scroller::Render()
	{
		// DEBUG
		RenderImGuiDebug();

		// TODO : Implement Scroller::Render()
		if (m_DebugRenderScrollArea)
		{
			DebugDraws::DrawScreenBoxMinMax_Pixels(
				m_TopLeftCorner, m_BottomRightCorner, glm::vec4(1.f, 0.f, 0.f, 1.0f), 4);
		}

		// Move Scroll depending on mouse position if dragging
		if (m_IsScrolling)
		{
			glm::ivec2 mousePosition = {s_InputsSnapshot->Mouse.Xpos, s_InputsSnapshot->Mouse.Ypos};
			float previousScrollRatio = m_ScrollRatio;
			m_ScrollRatio = GetScrollRatioFromMousePosition(mousePosition);
			if (m_ScrollRatio != previousScrollRatio)
			{
				OnScrollRatioChanged.Trigger(*this);
			}

			if (m_ScrollRatio == 0.0f)
			{
			}

			if (m_ScrollRatio == 1.0f)
			{
			}
		}

		// ---- Pull Events ---
		m_NineSliceSprite_Scroller.PullEvents();
		m_NineSliceSprite_ScrollerBackground.PullEvents();

		// ---- Calculations ----
		glm::ivec2 scrollerAreaSize = m_BottomRightCorner - m_TopLeftCorner;
		float scrollWidth = m_ScrollerWidthRatio * s_ScreenWidth;
		glm::ivec2 scrollBackgroundSize{static_cast<int>(scrollWidth), scrollerAreaSize.y};
		int scrollPosX = m_BottomRightCorner.x - scrollWidth / 2;
		int scrollBackgroundPosY = m_TopLeftCorner.y + scrollerAreaSize.y / 2;
		glm::ivec2 scrollerBackgroundPosition{scrollPosX, scrollBackgroundPosY};

		glm::ivec2 scrollSize = GetScrollHandleSize();
		int amplitude = scrollerAreaSize.y - scrollSize.y;
		int propAmp = static_cast<int>(amplitude * m_ScrollRatio);
		int scrollPosY = m_TopLeftCorner.y + propAmp + (scrollSize.y / 2.f);
		glm::ivec2 scrollPosition{scrollPosX, scrollPosY};

		// ---- Render Scroll Background ----
		m_NineSliceSprite_ScrollerBackground.SetPosition(scrollerBackgroundPosition);
		m_NineSliceSprite_ScrollerBackground.SetSize(scrollBackgroundSize);
		m_NineSliceSprite_ScrollerBackground.Render();

		// ---- Render Scroller ----
		m_NineSliceSprite_Scroller.SetPosition(scrollPosition);
		m_NineSliceSprite_Scroller.SetSize(scrollSize);
		m_NineSliceSprite_Scroller.Render();
	}

	void Scroller::Delete()
	{
		m_NineSliceSprite_Scroller.Delete();
		m_NineSliceSprite_ScrollerBackground.Delete();

		SetDeletedState(true);
	}

	void Scroller::ReloadTextures()
	{
		m_NineSliceSprite_Scroller.ReloadTextures();
		m_NineSliceSprite_ScrollerBackground.ReloadTextures();
	}

	void Scroller::StartCissoring()
	{
		glEnable(GL_SCISSOR_TEST);

		// Convert from top-left origin to bottom-left origin and flip Y axis
		int scissorX = m_TopLeftCorner.x;
		int scissorY = s_ScreenHeight - m_BottomRightCorner.y;
		int scissorWidth = m_BottomRightCorner.x - m_TopLeftCorner.x;
		int scissorHeight = m_BottomRightCorner.y - m_TopLeftCorner.y;

		glScissor(scissorX, scissorY, scissorWidth, scissorHeight);
	}

	void Scroller::StopCissoring()
	{
		glDisable(GL_SCISSOR_TEST);
	}

	void Scroller::SetTopLeftCorner(const glm::ivec2& topLeftCorner)
	{
		// If top left is greater than bottom right, we swap them to maintain a valid rectangle
		if (topLeftCorner.x > m_BottomRightCorner.x)
		{
			std::swap(m_TopLeftCorner.x, m_BottomRightCorner.x);
		}

		if (topLeftCorner.y > m_BottomRightCorner.y)
		{
			std::swap(m_TopLeftCorner.y, m_BottomRightCorner.y);
		}

		m_TopLeftCorner = topLeftCorner;
	}

	glm::ivec2 Scroller::GetTopLeftCorner() const
	{
		return m_TopLeftCorner;
	}

	void Scroller::SetBottomRightCorner(const glm::ivec2& bottomRightCorner)
	{
		// If bottom right is smaller than top left, we swap them to maintain a valid rectangle
		if (bottomRightCorner.x < m_TopLeftCorner.x)
		{
			std::swap(m_TopLeftCorner.x, m_BottomRightCorner.x);
		}

		if (bottomRightCorner.y < m_TopLeftCorner.y)
		{
			std::swap(m_TopLeftCorner.y, m_BottomRightCorner.y);
		}

		m_BottomRightCorner = bottomRightCorner;
	}

	glm::ivec2 Scroller::GetBottomRightCorner() const
	{
		return m_BottomRightCorner;
	}

	void Scroller::SetScrollAreaHeight(uint32_t scrollAreaHeight)
	{
		m_ScrollAreaHeight = scrollAreaHeight;
	}

	uint32_t Scroller::GetScrollAreaHeight() const
	{
		return m_ScrollAreaHeight;
	}

	uint32_t Scroller::GetContentYOffset() const
	{
		return static_cast<uint32_t>(m_ScrollRatio * m_ScrollAreaHeight);
	}

	glm::ivec2 Scroller::ProjectContentPosition(const glm::ivec2& contentPosition) const
	{
		uint32_t contentYOffset = GetContentYOffset();
		return contentPosition - glm::ivec2{0, static_cast<int>(contentYOffset)};
	}

	GuiElement::Visibility Scroller::GetControlVisibleArea(const glm::ivec2& controlCenter,
														   const glm::ivec2& controlSize) const
	{
		glm::ivec2 controlTopLeft = controlCenter - controlSize / 2;
		glm::ivec2 controlBottomRight = controlCenter + controlSize / 2;

		glm::ivec2 visibleTopLeft = glm::max(controlTopLeft, m_TopLeftCorner);
		glm::ivec2 visibleBottomRight = glm::min(controlBottomRight, m_BottomRightCorner);

		if (visibleBottomRight.x < visibleTopLeft.x || visibleBottomRight.y < visibleTopLeft.y)
		{
			return {false, false, {0, 0}, {0, 0}};
		}

		bool isFullyVisible = (visibleTopLeft == controlTopLeft) && (visibleBottomRight == controlBottomRight);

		return {true, isFullyVisible, visibleTopLeft, visibleBottomRight};
	}

	void Scroller::SetScrollRatio(float scrollRatio)
	{
		m_ScrollRatio = scrollRatio;
	}

	float Scroller::GetScrollRatio() const
	{
		return m_ScrollRatio;
	}

	void Scroller::SubscribeToSpriteEvents()
	{
		m_EventHandles.push_back(m_NineSliceSprite_Scroller.OnMouseDown.Subscribe([this](const NineSliceSprite& sprite)
																				  { Handle_MouseDown(sprite); }));

		m_EventHandles.push_back(m_NineSliceSprite_Scroller.OnMouseUp.Subscribe([this](const NineSliceSprite& sprite)
																				{ Handle_MouseUp(sprite); }));

		m_EventHandles.push_back(m_NineSliceSprite_ScrollerBackground.OnHoverEnter.Subscribe(
			[this](const NineSliceSprite& sprite) { Handle_HoverEnter(sprite); }));

		m_EventHandles.push_back(m_NineSliceSprite_ScrollerBackground.OnHoverLeave.Subscribe(
			[this](const NineSliceSprite& sprite) { Handle_HoverLeave(sprite); }));
	}

	void Scroller::Handle_MouseDown(const NineSliceSprite& sprite)
	{
		const auto& inputs = EngineContext::Get().Inputs;
		inputs->SetCursorStyle(CursorStyle::VResize);

		m_IsScrolling = true;

		m_MouseYOnScrollStart = s_InputsSnapshot->Mouse.Ypos;
		m_ScrollRatioOnDragStart = m_ScrollRatio;

		glm::ivec2 handleSize = GetScrollHandleSize();
		glm::ivec2 handleCenter = m_NineSliceSprite_Scroller.GetPosition();
		int handleTop = handleCenter.y - handleSize.y / 2;
		m_ClickOffsetInsideHandle = s_InputsSnapshot->Mouse.Ypos - handleTop;
	}

	void Scroller::Handle_MouseUp(const NineSliceSprite& sprite)
	{
		const auto& inputs = EngineContext::Get().Inputs;

		bool backgroundHovered = m_NineSliceSprite_ScrollerBackground.IsHovered();
		if (backgroundHovered)
		{
			inputs->SetCursorStyle(CursorStyle::Hand);
		}
		else
		{
			inputs->SetCursorStyle(CursorStyle::Arrow);
		}

		m_IsScrolling = false;
	}

	void Scroller::Handle_HoverEnter(const NineSliceSprite& sprite)
	{
		const auto& inputs = EngineContext::Get().Inputs;
		inputs->SetCursorStyle(CursorStyle::Hand);
	}

	void Scroller::Handle_HoverLeave(const NineSliceSprite& sprite)
	{
		const auto& inputs = EngineContext::Get().Inputs;
		inputs->SetCursorStyle(CursorStyle::Arrow);
	}

	glm::ivec2 Scroller::GetScrollHandleSize() const
	{
		int scrollerAreaSizeY = m_BottomRightCorner.y - m_TopLeftCorner.y;
		float scrollHeightRatio;
		if (m_ScrollAreaHeight == 0)
		{
			scrollHeightRatio = 1.f;
		}
		else
		{
			scrollHeightRatio = static_cast<float>(scrollerAreaSizeY) / m_ScrollAreaHeight;
			scrollHeightRatio = std::min(scrollHeightRatio, 1.f);
		}
		int scrollHeight = static_cast<int>(scrollHeightRatio * scrollerAreaSizeY);
		int scrollWidth = static_cast<int>(m_ScrollerWidthRatio * s_ScreenWidth);
		return {scrollWidth, scrollHeight};
	}

	float Scroller::GetScrollRatioFromMousePosition(const glm::ivec2& mousePosition) const
	{
		int scrollerAreaSizeY = m_BottomRightCorner.y - m_TopLeftCorner.y;

		if (scrollerAreaSizeY <= 0)
			return 0.0f;

		int deltaY = mousePosition.y - m_MouseYOnScrollStart;

		float scrollHeight = GetScrollHandleSize().y;

		float amplitude = scrollerAreaSizeY - scrollHeight;

		if (amplitude <= 0.0f)
			return 0.0f;

		float deltaRatio = static_cast<float>(deltaY) / amplitude;

		float newRatio = m_ScrollRatioOnDragStart + deltaRatio;

		return std::clamp(newRatio, 0.0f, 1.0f);
	}

	void Scroller::RenderImGuiDebug()
	{
		// Debug ImGui pannel
		ImGui::Begin(("Scroller: " + GetName()).c_str());

		ImGui::Text("Scroller Hovered: %s", m_NineSliceSprite_Scroller.IsHovered() ? "Yes" : "No");
		ImGui::Text("Background Hovered: %s", m_NineSliceSprite_ScrollerBackground.IsHovered() ? "Yes" : "No");

		ImGui::Text("Scrolling: %s", m_IsScrolling ? "Yes" : "No");

		ImGui::Separator();

		ImGui::Text("Top Left Corner: (%d, %d)", m_TopLeftCorner.x, m_TopLeftCorner.y);
		ImGui::Text("Bottom Right Corner: (%d, %d)", m_BottomRightCorner.x, m_BottomRightCorner.y);
		ImGui::Text("Scroll Area Height: %d", m_ScrollAreaHeight);

		ImGui::Checkbox("Show Scroll Area", &m_DebugRenderScrollArea);

		ImGui::Separator();

		ImGui::SliderFloat("Scroll Ratio", &m_ScrollRatio, 0.0f, 1.0f);

		ImGui::End();
	}

} // namespace onion::voxel
