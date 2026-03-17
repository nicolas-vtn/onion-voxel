#pragma once

#include <glm/glm.hpp>

#include <string>
#include <vector>

#include <onion/Event.hpp>

#include <renderer/OpenGL.hpp>

#include <renderer/gui/GuiElement.hpp>
#include <renderer/gui/controls/label/Label.hpp>
#include <renderer/gui/nine_slice_sprite/NineSliceSprite.hpp>

namespace onion::voxel
{
	class Slider : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		Slider(const std::string& name);
		~Slider();

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

		/// @brief Sets the size of the slider. (In Pixels)
		/// @param size The new size of the slider in pixels.
		void SetSize(const glm::ivec2& size);
		glm::ivec2 GetSize() const;

		/// @brief Sets the position of the slider. (In Pixels)
		/// @param pos The new position of the slider in pixels.
		void SetPosition(const glm::ivec2& pos);
		glm::ivec2 GetPosition() const;

		/// @brief Sets the current value of the slider. Maximum resolution is 1. The slider will snap to the nearest integer value.
		void SetValue(uint32_t value);
		uint32_t GetValue() const;

		/// @brief Sets the maximum value of the slider. Minimum value is always 0. Maximum resolution is 1. The slider will snap to the nearest integer value.
		void SetMaxValue(uint32_t maxValue);
		uint32_t GetMaxValue() const;

		// ----- Events -----
	  public:
		Event<const Slider&> OnValueChanged;

	  private:
		void SubscribeToSpriteEvents();

		std::vector<EventHandle> m_EventHandles;

		void Handle_MouseDown(const NineSliceSprite& sprite);
		void Handle_MouseUp(const NineSliceSprite& sprite);
		void Handle_HoverEnter(const NineSliceSprite& sprite);
		void Handle_HoverLeave(const NineSliceSprite& sprite);

		// ----- Properties -----
	  private:
		glm::ivec2 m_Position{0, 0};
		glm::ivec2 m_Size{1, 1};

		bool m_IsSliding = false;

		uint32_t m_PrevValue = 0;
		uint32_t m_Value = 0;
		uint32_t m_MaxValue = 100;

		float m_TextHeightRatio = 0.390f;
		int m_HandleWidth = 32;

		uint32_t GetValueFromMousePosition(const glm::ivec2& mousePosition) const;

		// ----- Label -----
	  private:
		Label m_Label;

		// ----- NineSliceSprites -----
	  private:
		NineSliceSprite m_NineSliceSprite_SliderBasic;
		NineSliceSprite m_NineSliceSprite_SliderHighlighted;
		NineSliceSprite m_NineSliceSprite_HandleBasic;
		NineSliceSprite m_NineSliceSprite_HandleHighlighted;

		// ----- Static Helpers -----
	  private:
		static inline const std::filesystem::path s_SpritePathFromGui_Slider =
			GuiElement::s_BasePathGuiAssets / "sprites" / "widget" / "slider.png";

		static inline const std::filesystem::path s_SpritePathFromGui_SliderHighlighted =
			GuiElement::s_BasePathGuiAssets / "sprites" / "widget" / "slider_highlighted.png";

		static inline const std::filesystem::path s_SpritePathFromGui_HandleBasic =
			GuiElement::s_BasePathGuiAssets / "sprites" / "widget" / "slider_handle.png";

		static inline const std::filesystem::path s_SpritePathFromGui_HandleHighlighted =
			GuiElement::s_BasePathGuiAssets / "sprites" / "widget" / "slider_handle_highlighted.png";

		static inline constexpr glm::vec3 s_TextColor = glm::vec3(1.f);
		static inline constexpr glm::vec3 s_TextShadowColor = glm::vec3(63.f / 255.f);

		// ----- DEBUG -----
	  private:
		void RenderImGuiDebug();
	};
} // namespace onion::voxel
