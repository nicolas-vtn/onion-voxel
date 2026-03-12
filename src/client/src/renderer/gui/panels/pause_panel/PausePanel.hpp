#pragma once

#include "../../GuiElement.hpp"
#include "../../controls/button/Button.hpp"
#include "../../controls/label/Label.hpp"

namespace onion::voxel
{
	class PausePanel : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		PausePanel(const std::string& name);
		~PausePanel() override;

		// ----- Public API -----
	  public:
		void Render() override;
		void Initialize() override;
		void Delete() override;

		// ----- Public Events -----
	  public:
		Event<const GuiElement*> RequestBackToGame;
		Event<const std::pair<const GuiElement*, eMenu>&> RequestMenuNavigation;
		Event<const GuiElement*> RequestQuitToMainMenu;

		// ----- Controls -----
	  private:
		Label m_Title_Label;
		Button m_BackToGame_Button;
		Button m_Advancements_Button;
		Button m_Statistics_Button;
		Button m_GiveFeedback_Button;
		Button m_ReportBugs_Button;
		Button m_Options_Button;
		Button m_OpenToLan_Button;
		Button m_MainMenu_Button;

		// ----- Internal Event Subscription and Handlers -----
	  private:
		std::vector<EventHandle> m_EventHandles;
		void SubscribeToControlEvents();

		void Handle_BackToGame_Click(const Button& sender);
		void Handle_Options_Click(const Button& sender);
		void Handle_MainMenu_Click(const Button& sender);
	};
} // namespace onion::voxel
