#pragma once

#include <renderer/gui/GuiElement.hpp>
#include <renderer/gui/controls/button/Button.hpp>
#include <renderer/gui/controls/label/Label.hpp>
#include <renderer/gui/controls/scroller/Scroller.hpp>
#include <renderer/gui/controls/slider/Slider.hpp>

namespace onion::voxel
{
	class MouseSettingsPanel : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		MouseSettingsPanel(const std::string& name);
		~MouseSettingsPanel() override;

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

		// ----- Controls -----
	  private:
		Label m_Title_Label;
		Scroller m_Scroller;

		Button m_Done_Button;

		// ----- States -----
	  private:
		// ----- Internal Event Subscription and Handlers -----
	  private:
		std::vector<EventHandle> m_EventHandles;
		void SubscribeToControlEvents();

		void Handle_Done_Click(const Button& sender);
	};
} // namespace onion::voxel
