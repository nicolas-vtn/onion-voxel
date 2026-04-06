#pragma once

#include <renderer/gui/GuiElement.hpp>
#include <renderer/gui/controls/button/Button.hpp>
#include <renderer/gui/controls/label/Label.hpp>
#include <renderer/gui/controls/slider/Slider.hpp>

namespace onion::voxel
{
	class OptionsPanel : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		OptionsPanel(const std::string& name);
		~OptionsPanel() override;

		// ----- Public API -----
	  public:
		void Render() override;
		void Initialize() override;
		void Delete() override;
		void ReloadTextures() override;

		// ----- Public Events -----
	  public:
		Event<const GuiElement*> RequestBackNavigation;
		Event<const std::pair<const GuiElement*, eMenu>&> RequestMenuNavigation;
		Event<const UserSettingsChangedEventArgs&> EvtUserSettingsChanged;

		// ----- Controls -----
	  private:
		Label m_Title_Label;
		Slider m_Fov_Slider;
		Button m_Online_Button;
		Button m_SkinCustomization_Button;
		Button m_MusicAndSounds_Button;
		Button m_VideoSettings_Button;
		Button m_Controls_Button;
		Button m_Language_Button;
		Button m_ChatSettings_Button;
		Button m_ResourcePacks_Button;
		Button m_AccessibilitySettings_Button;
		Button m_Telemetry_Button;
		Button m_Credits_Button;
		Button m_Done_Button;

		// ----- Parameters -----
	  private:
		static const int s_FovMinValue = 30;
		static const int s_FovMaxValue = 110;

		// ----- Internal Event Subscription and Handlers -----
	  private:
		std::vector<EventHandle> m_EventHandles;
		void SubscribeToControlEvents();

		void Handle_MusicAndSounds_Click(const Button& sender);
		void Handle_Controls_Click(const Button& sender);
		void Handle_ResourcePacks_Click(const Button& sender);
		void Handle_Done_Click(const Button& sender);
		void Handle_Fov_Slider_ValueChanged(const Slider& sender);
		void Handle_VideoSettings_Click(const Button& sender);
	};
} // namespace onion::voxel
