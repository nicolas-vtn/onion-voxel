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

		Slider m_MouseSensitivity_Slider;
		Button m_TouchscreenMode_Button;
		Slider m_MouseScrollSensitivity_Slider;
		Button m_DiscreteScroll_Button;
		Button m_InvertMouseX_Button;
		Button m_InvertMouseY_Button;
		Button m_AllowCursorChanges_Button;
		Button m_RawInput_Button;

		Button m_Done_Button;

		// ----- Settings -----
	  private:
		static inline const float s_MouseSensitivityReferenceValue = 0.1f; // This is the value that corresponds to 100%

		static inline const float s_MouseScrollSensitivity_MinValue = 0.01f;
		static inline const float s_MouseScrollSensitivity_MaxValue = 3.f;

		// ----- States -----
	  private:
		// ----- Internal Event Subscription and Handlers -----
	  private:
		std::vector<EventHandle> m_EventHandles;
		void SubscribeToControlEvents();

		void Handle_MouseSensitivity_Changed(const Slider& sender);
		void Handle_MouseScrollSensitivity_Changed(const Slider& sender);

		void Handle_Done_Click(const Button& sender);
	};
} // namespace onion::voxel
