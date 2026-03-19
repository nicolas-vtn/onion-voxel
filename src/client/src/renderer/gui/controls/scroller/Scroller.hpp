#pragma once

#include <glm/glm.hpp>

#include <string>
#include <vector>

#include <onion/Event.hpp>

#include <renderer/OpenGL.hpp>

#include <renderer/gui/GuiElement.hpp>
#include <renderer/gui/nine_slice_sprite/NineSliceSprite.hpp>

namespace onion::voxel
{
	class Scroller : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		Scroller(const std::string& name);
		~Scroller();

		// ----- Public API -----
	  public:
		void Initialize() override;
		void Render() override;
		void Delete() override;
		void ReloadTextures() override;

		void StartCissoring();
		void StopCissoring();

		// ----- Getters / Setters -----
	  public:
		void SetTopLeftCorner(const glm::ivec2& topLeftCorner);
		glm::ivec2 GetTopLeftCorner() const;

		void SetBottomRightCorner(const glm::ivec2& bottomRightCorner);
		glm::ivec2 GetBottomRightCorner() const;

		void SetScrollAreaHeight(uint32_t scrollAreaHeight);
		uint32_t GetScrollAreaHeight() const;

		uint32_t GetContentYOffset() const;

		glm::ivec2 ProjectContentPosition(const glm::ivec2& contentPosition) const;
		Visibility GetControlVisibleArea(const glm::ivec2& controlCenter, const glm::ivec2& controlSize) const;

		void SetScrollRatio(float scrollRatio);
		float GetScrollRatio() const;

		// ----- Events -----
	  public:
		Event<const Scroller&> OnScrollRatioChanged;

	  private:
		void SubscribeToSpriteEvents();

		std::vector<EventHandle> m_EventHandles;

		void Handle_MouseDown(const NineSliceSprite& sprite);
		void Handle_MouseUp(const NineSliceSprite& sprite);
		void Handle_HoverEnter(const NineSliceSprite& sprite);
		void Handle_HoverLeave(const NineSliceSprite& sprite);

		// ----- Properties -----
	  private:
		glm::ivec2 m_TopLeftCorner{0, 0};
		glm::ivec2 m_BottomRightCorner{0, 0};
		uint32_t m_ScrollAreaHeight = 0;

		float m_ScrollRatio = 0.0f;

		bool m_IsScrolling = false;
		int m_MouseYOnScrollStart = 0;
		int m_MouseOffsetInHandle;
		float m_ScrollRatioOnDragStart = 0.f;
		int m_ClickOffsetInsideHandle;

		bool m_DebugRenderScrollArea = false;

		float m_ScrollerWidthRatio = 30.f / 1920.f;

		glm::ivec2 GetScrollHandleSize() const;
		float GetScrollRatioFromMousePosition(const glm::ivec2& mousePosition) const;
		void HandleMouseScrollY();

		// ----- NineSliceSprites -----
	  private:
		NineSliceSprite m_NineSliceSprite_Scroller;
		NineSliceSprite m_NineSliceSprite_ScrollerBackground;

		// ----- Static Helpers -----
	  private:
		static inline const std::filesystem::path s_SpritePathFromGui_Scroller =
			GuiElement::s_BasePathGuiAssets / "sprites" / "widget" / "scroller.png";

		static inline const std::filesystem::path s_SpritePathFromGui_ScrollerBackground =
			GuiElement::s_BasePathGuiAssets / "sprites" / "widget" / "scroller_background.png";

		// ----- DEBUG -----
	  private:
		void RenderImGuiDebug();
	};
} // namespace onion::voxel
