#pragma once

#include "layouts/demo_panel/DemoPanel.hpp"
#include "layouts/main_menu_panel/MainMenuPanel.hpp"
#include "layouts/options/OptionsPanel.hpp"
#include "layouts/pause_panel/PausePanel.hpp"

#include <memory>
#include <mutex>
#include <stack>

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

		// ----- Getters / Setters -----
	  public:
		void SetActiveMenu(eMenu menu);
		void GoBackToPreviousMenu();
		eMenu GetActiveMenu() const;
		void SetGuiScale(int scale);
		int GetGuiScale() const;

		// ----- Events -----
	  public:
		Event<const CursorStyle&> RequestCursorStyleChange;
		Event<const std::filesystem::path&> RequestStartSingleplayerGame;
		Event<bool> RequestQuitToMainMenu;
		Event<bool> RequestBackToGame;
		Event<bool> RequestBack;

		// ----- Panels -----
	  private:
		DemoPanel m_DemoPanel;
		MainMenuPanel m_MainMenuPanel;
		PausePanel m_PausePanel;
		OptionsPanel m_OptionsPanel;

		// ----- Panel Events Handling -----
	  private:
		std::vector<EventHandle> m_EventHandles;
		void SubscribeToPannelsEvents();

		void Handle_MenuNavigationRequest(const std::pair<const GuiElement*, eMenu>& request);
		void Handle_QuitGameRequest(const GuiElement* sender);
		void Handle_CursorStyleChangeRequest(const CursorStyle& style);
		void Handle_BackToGameRequest(const GuiElement* sender);
		void Handle_QuitToMainMenuRequest(const GuiElement* sender);
		void Handle_BackRequest(const GuiElement* sender);

		// ----- Set Static States -----
	  public:
		static void SetInputsSnapshot(std::shared_ptr<InputsSnapshot> inputsSnapshot);
		static void SetScreenSize(int screenWidth, int screenHeight);

		// ----- States -----
	  private:
		mutable std::mutex m_MutexState;
		eMenu m_ActiveMenu = eMenu::MainMenu;
		std::stack<eMenu> m_MenuHistory;

		// ----- ImGui menu -----
	  private:
		void RenderDebugPanel();
	};
} // namespace onion::voxel
