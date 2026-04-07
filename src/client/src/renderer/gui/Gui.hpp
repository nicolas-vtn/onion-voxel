#pragma once

#include <memory>
#include <mutex>
#include <stack>

#include <user_settings/UserSettings.hpp>

#include <renderer/camera/Camera.hpp>
#include <renderer/skybox/Skybox.hpp>

#include "panels/controls_panel/ControlsPanel.hpp"
#include "panels/demo_panel/DemoPanel.hpp"
#include "panels/demo_scrolling_panel/DemoScrollingPanel.hpp"
#include "panels/demo_texts_panel/DemoTextsPanel.hpp"
#include "panels/key_binds_panel/KeyBindsPanel.hpp"
#include "panels/main_menu_panel/MainMenuPanel.hpp"
#include "panels/mouse_settings_panel/MouseSettingsPanel.hpp"
#include "panels/options_panel/OptionsPanel.hpp"
#include "panels/pause_panel/PausePanel.hpp"
#include "panels/resource_packs_panel/ResourcePacksPanel.hpp"
#include "panels/singleplayer_panel/SingleplayerPanel.hpp"
#include "panels/video_settings_panel/VideoSettingsPanel.hpp"

namespace onion::voxel
{
	class Gui
	{
	  public:
		struct MultiplayerGameStartInfo
		{
			std::string ServerAddress;
			uint16_t ServerPort;
		};

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
		void RenderBackground();
		void Shutdown();
		void ReloadTextures();

		// ----- Getters / Setters -----
	  public:
		void SetActiveMenu(eMenu menu, bool withHistory = true);
		void GoBackToPreviousMenu();
		eMenu GetActiveMenu() const;
		void SetGuiScale(int scale);
		int GetGuiScale() const;
		void SetIsInGame(bool isInGame);

		// ----- Events -----
	  public:
		Event<const CursorStyle&> RequestCursorStyleChange;
		Event<const WorldInfos&> RequestStartSingleplayerGame;
		Event<const MultiplayerGameStartInfo&> RequestStartMultiplayerGame;
		Event<bool> RequestQuitToMainMenu;
		Event<bool> RequestBackToGame;
		Event<bool> RequestBack;
		Event<const std::string&> RequestResourcePackChange;
		Event<const UserSettingsChangedEventArgs&> UserSettingsChanged;

		// ----- Panels -----
	  private:
		DemoPanel m_DemoPanel;
		DemoScrollingPanel m_DemoScrollingPanel;
		DemoTextsPanel m_DemoTextsPanel;
		MainMenuPanel m_MainMenuPanel;
		PausePanel m_PausePanel;
		OptionsPanel m_OptionsPanel;
		VideoSettingsPanel m_VideoSettingsPanel;
		ResourcePacksPanel m_ResourcePacksPanel;
		SingleplayerPanel m_SingleplayerPanel;
		ControlsPanel m_ControlsPanel;
		MouseSettingsPanel m_MouseSettingsPanel;
		KeyBindsPanel m_KeyBindsPanel;

		// ---- Private Members -----
	  private:
		bool m_IsInGame = false;
		Skybox m_Skybox;
		static Camera s_Camera; // Camera for Skybox.
		static inline float s_CameraFov{90.f};
		static inline float s_SkyboxRotationPeriod{400.0f}; // Time in seconds for a full rotation
		void ReloadSkyboxTextures();

		// ----- Panel Events Handling -----
	  private:
		std::vector<EventHandle> m_EventHandles;
		void SubscribeToPanelsEvents();

		void Handle_MenuNavigationRequest(const std::pair<const GuiElement*, eMenu>& request);
		void Handle_QuitGameRequest(const GuiElement* sender);
		void Handle_CursorStyleChangeRequest(const CursorStyle& style);
		void Handle_BackToGameRequest(const GuiElement* sender);
		void Handle_QuitToMainMenuRequest(const GuiElement* sender);
		void Handle_BackRequest(const GuiElement* sender);
		void Handle_ResourcePackChangeRequest(const std::string& resourcePackName);
		void Handle_UserSettingsChanged(const UserSettingsChangedEventArgs& eventArgs);

		// ----- Set Static States -----
	  public:
		static void SetInputsSnapshot(std::shared_ptr<InputsSnapshot> inputsSnapshot);

		static inline EventHandle s_HandleFramebufferResized;
		static void Handle_FramebufferResized(const FramebufferState& framebufferState);

		// ----- States -----
	  private:
		mutable std::mutex m_MutexState;
		eMenu m_ActiveMenu = eMenu::MainMenu;
		std::stack<eMenu> m_MenuHistory;
		eMenu m_MenuPreviousFrame = eMenu::None;

		// ----- ImGui menu -----
	  private:
		void RenderDebugPanel();
	};

} // namespace onion::voxel
