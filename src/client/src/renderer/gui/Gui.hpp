#pragma once

#include "layouts/demo_panel/DemoPanel.hpp"
#include "layouts/main_menu_panel/MainMenuPanel.hpp"

#include <memory>
#include <mutex>

namespace onion::voxel
{
	class Gui
	{
		// ----- Static Initialization / Shutdown -----
	  public:
		static void StaticInitialize();
		static void StaticShutdown();

		// ----- Constructor / Destructor -----
	  public:
		Gui();
		~Gui();

		// ----- Public API -----
	  public:
		void Initialize();
		void Render();
		void Shutdown();

		void SetActiveMenu(eMenu menu);
		eMenu GetActiveMenu() const;
		void SetGameVersion(const std::string& version);

		// ----- Panels -----
	  private:
		DemoPanel m_DemoPanel;
		MainMenuPanel m_MainMenuPanel;

		// ----- Panel Events Handling -----
	  private:
		std::vector<EventHandle> m_EventHandles;
		void SubscribeToPannelsEvents();

		void Handle_MenuNavigationRequest(const eMenu& menu);
		void Handle_QuitGameRequest(const GuiElement* sender);
		// ----- Set Static States -----
	  public:
		static void SetInputsSnapshot(std::shared_ptr<InputsSnapshot> inputsSnapshot);
		static void SetScreenSize(int screenWidth, int screenHeight);

		// ----- States -----
	  private:
		mutable std::mutex m_MutexState;
		eMenu m_ActiveMenu = eMenu::MainMenu;
	};
} // namespace onion::voxel
