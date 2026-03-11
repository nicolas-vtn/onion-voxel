#pragma once

#include "../../GuiElement.hpp"
#include "../../controls/button/Button.hpp"

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

		// ----- Public Events -----
	  public:
		Event<const GuiElement*> RequestBackNavigation;
		Event<const std::pair<const GuiElement*, eMenu>&> RequestMenuNavigation;

		// ----- Controls -----
	  private:
		Button m_FOV_Button;
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

		// ----- Internal Event Subscription and Handlers -----
	  private:
		std::vector<EventHandle> m_EventHandles;
		void SubscribeToControlEvents();

		void Handle_MusicAndSounds_Click(const Button& sender);
		void Handle_Controls_Click(const Button& sender);
		void Handle_ResourcePacks_Click(const Button& sender);
		void Handle_Done_Click(const Button& sender);
	};
} // namespace onion::voxel
