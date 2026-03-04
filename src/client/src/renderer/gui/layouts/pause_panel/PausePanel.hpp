#pragma once

#include "../../GuiElement.hpp"
#include "../../controls/button/Button.hpp"

namespace onion::voxel
{
	class PausePanel : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		PausePanel(const std::string& name);
		~PausePanel() override = default;

		// ----- Public API -----
	  public:
		void Render() override;
		void Initialize() override;
		void Delete() override;

		// ----- Public Events -----
	  public:
		Event<bool> RequestGameResume;
		Event<eMenu> RequestMenuNavigation;
		Event<bool> RequestQuitToMainMenu;

		// ----- Controls -----
	  private:
		Button m_Resume_Button;
		Button m_Options_Button;
		Button m_MainMenu_Button;

		// ----- Internal Event Subscription and Handlers -----
	  private:
		std::vector<EventHandle> m_EventHandles;
		void SubscribeToControlEvents();

		void Handle_Resume_Click(const Button& sender);
		void Handle_Options_Click(const Button& sender);
		void Handle_MainMenu_Click(const Button& sender);
	};
} // namespace onion::voxel
