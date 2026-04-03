#pragma once

#include <renderer/gui/GuiElement.hpp>
#include <renderer/gui/controls/button/Button.hpp>
#include <renderer/gui/controls/checkbox/Checkbox.hpp>
#include <renderer/gui/controls/label/Label.hpp>
#include <renderer/gui/controls/scroller/Scroller.hpp>
#include <renderer/gui/controls/slider/Slider.hpp>
#include <renderer/gui/controls/sprite/Sprite.hpp>
#include <renderer/gui/controls/text_field/TextField.hpp>

namespace onion::voxel
{
	class DemoScrollingPanel : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		DemoScrollingPanel(const std::string& name);
		~DemoScrollingPanel() override;

		// ----- Public API -----
	  public:
		void Render() override;
		void Initialize() override;
		void Delete() override;
		void ReloadTextures() override;

		// ----- Public Events -----
	  public:
		Event<const GuiElement*> RequestBackNavigation;

		// ----- Properties -----
	  private:
		std::string m_SpritePath = (AssetsManager::GetTexturesDirectory() / "OnionVoxelTitle.png").string();

		// ----- Controls -----
	  private:
		Button m_ButtonBack;
		Slider m_SliderButtonCount;
		Scroller m_Scroller;

		Slider m_SliderDemo;
		Label m_LabelDemo;
		Sprite m_SpriteDemo;
		Checkbox m_CheckboxDemo;
		TextField m_TextFieldDemo;
		std::vector<std::unique_ptr<Button>> m_DynamicButtons;

		void AdjustDynamicButtons(size_t buttonCount);

		// ----- Internal Event Subscription and Handlers -----
	  private:
		void SubscribeToControlEvents();

		std::vector<EventHandle> m_EventHandles;

		void Handle_ButtonBackClick(const Button& button);
		void Handle_SliderButtonCountValueChanged(const Slider& slider);
		void Handle_ScrollerScrollRatioChanged(const Scroller& scroller);
	};
} // namespace onion::voxel
