#pragma once

#include <glm/glm.hpp>

#include <string>
#include <vector>

#include <onion/Event.hpp>

#include <renderer/OpenGL.hpp>

#include <renderer/gui/GuiElement.hpp>
#include <renderer/gui/controls/label/Label.hpp>
#include <renderer/gui/nine_slice_sprite/NineSliceSprite.hpp>
#include <renderer/texture/texture.hpp>

namespace onion::voxel
{
	class Button : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		Button(const std::string& name);
		~Button();

		// ----- Public API -----
	  public:
		void Initialize() override;
		void Render() override;
		void Delete() override;
		void ReloadTextures() override;

		// ----- Getters / Setters -----
	  public:
		void SetText(const std::string& text);
		std::string GetText() const;

		/// @brief Sets the size of the button. (In Pixels)
		/// @param size The new size of the button in pixels.
		void SetSize(const glm::ivec2& size);
		glm::ivec2 GetSize() const;

		/// @brief Sets the position of the button. (In Pixels)
		/// @param pos The new position of the button in pixels.
		void SetPosition(const glm::ivec2& pos);
		glm::ivec2 GetPosition() const;

		void SetVisibility(const Visibility& visibility) override;

		bool IsEnabled() const;
		void SetEnabled(bool enabled);

		// ----- Events -----
	  public:
		Event<const Button&> OnClick;
		Event<const Button&> OnHoverEnter;
		Event<const Button&> OnHoverLeave;

	  private:
		void SubscribeToSpriteEvents();

		EventHandle m_HandleMouseDown;
		void HandleMouseDown(const NineSliceSprite& sprite);
		EventHandle m_HandleMouseUp;
		void HandleMouseUp(const NineSliceSprite& sprite);
		EventHandle m_HandleSpriteClick;
		void HandleSpriteClick(const NineSliceSprite& sprite);
		EventHandle m_HandleSpriteHoverEnter;
		void HandleSpriteHoverEnter(const NineSliceSprite& sprite);
		EventHandle m_HandleSpriteHoverLeave;
		void HandleSpriteHoverLeave(const NineSliceSprite& sprite);

		// ----- Properties -----
	  private:
		bool m_IsEnabled = true;

		glm::ivec2 m_Position{0, 0};
		glm::ivec2 m_Size{1, 1};
		bool m_IsPressed = false;
		float m_ScaleFactorOnClick = 0.95f;

		// ----- Label -----
	  private:
		Label m_Label;

		// ----- NineSliceSprites -----
	  private:
		NineSliceSprite m_NineSliceSprite_Basic;
		NineSliceSprite m_NineSliceSprite_Disabled;
		NineSliceSprite m_NineSliceSprite_Highlighted;

		// ----- Static Helpers -----
	  private:
		static inline const std::filesystem::path s_SpritePathFromGui =
			GuiElement::s_BasePathGuiAssets / "sprites" / "widget" / "button.png";

		static inline const std::filesystem::path s_SpritePathFromGui_Disabled =
			GuiElement::s_BasePathGuiAssets / "sprites" / "widget" / "button_disabled.png";

		static inline const std::filesystem::path s_SpritePathFromGui_Highlighted =
			GuiElement::s_BasePathGuiAssets / "sprites" / "widget" / "button_highlighted.png";

		// ----- DEBUG -----
	  private:
		void RenderImGuiDebug();
	};
} // namespace onion::voxel
