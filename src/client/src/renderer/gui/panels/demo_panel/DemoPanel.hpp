#pragma once

#include <renderer/gui/GuiElement.hpp>
#include <renderer/gui/controls/button/Button.hpp>
#include <renderer/gui/controls/checkbox/Checkbox.hpp>
#include <renderer/gui/controls/slider/Slider.hpp>
#include <renderer/gui/controls/sprite/Sprite.hpp>
#include <renderer/gui/controls/text_field/TextField.hpp>

namespace onion::voxel
{
	class DemoPanel : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		DemoPanel(const std::string& name);
		~DemoPanel() override;

		// ----- Public API -----
	  public:
		void Render() override;
		void Initialize() override;
		void Delete() override;
		void ReloadTextures() override;

		// ----- Public Events -----
	  public:
		Event<const std::pair<const GuiElement*, eMenu>&> RequestMenuNavigation;

		// ----- Properties -----
	  private:
		std::string m_SpritePath = (AssetsManager::GetTexturesDirectory() / "OnionVoxelTitle.png").string();

		// ----- Controls -----
	  private:
		Button m_Button;
		Button m_Button2;
		Button m_ButtonMainMenu;
		Button m_ButtonScrollingPanel;
		Sprite m_Sprite;
		Checkbox m_Checkbox;
		TextField m_TextField;
		Slider m_Slider;

		// ----- Internal Event Subscription and Handlers -----
	  private:
		void SubscribeToControlEvents();

		std::vector<EventHandle> m_EventHandles;

		void Handle_ButtonClick(const Button& button);
		void Handle_ButtonHoverEnter(const Button& button);
		void Handle_ButtonHoverLeave(const Button& button);
		void Handle_ButtonMainMenuClick(const Button& button);
		void Handle_CheckboxCheckedChanged(const Checkbox& checkbox);
		void Handle_SliderValueChanged(const Slider& slider);
		void Handle_ButtonScrollingPanelClick(const Button& button);
	};
} // namespace onion::voxel
