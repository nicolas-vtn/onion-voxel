#pragma once

#include <renderer/gui/GuiElement.hpp>
#include <renderer/gui/controls/button/Button.hpp>
#include <renderer/gui/controls/label/Label.hpp>
#include <renderer/gui/controls/scroller/Scroller.hpp>
#include <renderer/gui/controls/slider/Slider.hpp>

namespace onion::voxel
{
	class ControlsPanel : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		ControlsPanel(const std::string& name);
		~ControlsPanel() override;

		// ----- Public API -----
	  public:
		void Render() override;
		void Initialize() override;
		void Delete() override;
		void ReloadTextures() override;

		// ----- Public Events -----
	  public:
		Event<const GuiElement*> EvtRequestBackNavigation;
		Event<const UserSettingsChangedEventArgs&> EvtUserSettingsChanged;
		Event<const std::pair<const GuiElement*, eMenu>&> EvtRequestMenuNavigation;

		// ----- Controls -----
	  private:
		Label m_Title_Label;
		Scroller m_Scroller;

		Button m_MouseSettings_Button;
		Button m_KeyBinds_Button;
		Button m_Sneak_Button;
		Button m_Sprint_Button;
		Button m_AtkDestroy_Button;
		Button m_Use_Button;
		Button m_AutoJump_Button;
		Slider m_SprintWindow_Slider;
		Button m_OperatorItemTabs_Button;

		Button m_Done_Button;

		// ----- Settings -----
	  private:
		// ----- States -----
	  private:
		// ----- Internal Event Subscription and Handlers -----
	  private:
		std::vector<EventHandle> m_EventHandles;
		void SubscribeToControlEvents();

		void Handle_MouseSettings_Click(const Button& sender);
		void Handle_KeyBinds_Click(const Button& sender);
		void Handle_Done_Click(const Button& sender);
	};
} // namespace onion::voxel
