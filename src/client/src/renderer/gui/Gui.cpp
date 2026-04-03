#include "Gui.hpp"

#include <imgui.h>

#include <csignal>

namespace onion::voxel
{
	Camera Gui::s_Camera(glm::vec3(0.f), 800, 600);

	void Gui::StaticInitialize()
	{
		GuiElement::Load();

		const auto& inputsManager = EngineContext::Get().Inputs;
		s_HandleFramebufferResized = inputsManager->EventFramebufferResized.Subscribe(
			[](const FramebufferState& framebufferState) { Handle_FramebufferResized(framebufferState); });

		// Initialize the framebuffer size based on the current state of the InputsManager
		FramebufferState framebufferState = inputsManager->GetFramebufferState();
		Handle_FramebufferResized(framebufferState);
	}

	void Gui::StaticShutdown()
	{
		GuiElement::Unload();
	}

	Gui::Gui()
		: m_DemoPanel("DemoPanel"), m_MainMenuPanel("MainMenuPanel"), m_PausePanel("PausePanel"),
		  m_OptionsPanel("OptionsPanel"), m_ResourcePacksPanel("ResourcePacksPanel"),
		  m_DemoScrollingPanel("DemoScrollingPanel"), m_SingleplayerPanel("SingleplayerPanel"),
		  m_VideoSettingsPanel("VideoSettingsPanel"), m_ControlsPanel("ControlsPanel"),
		  m_MouseSettingsPanel("MouseSettingsPanel"), m_KeyBindsPanel("KeyBindsPanel")
	{
		SubscribeToPanelsEvents();
	}

	Gui::~Gui()
	{
		m_EventHandles.clear();
	}

	void Gui::ReloadSkyboxTextures()
	{
		// Reload the skybox textures.
		auto pathPanorama = std::filesystem::path("assets") / "minecraft" / "textures" / "gui" / "title" / "background";
		auto pathPano0 = pathPanorama / "panorama_0.png";
		auto pathPano1 = pathPanorama / "panorama_1.png";
		auto pathPano2 = pathPanorama / "panorama_2.png";
		auto pathPano3 = pathPanorama / "panorama_3.png";
		auto pathPano4 = pathPanorama / "panorama_4.png";
		auto pathPano5 = pathPanorama / "panorama_5.png";

		auto assetsManager = EngineContext::Get().Assets;
		std::vector<uint8_t> dataPano0 = assetsManager->GetResourcePackFileBinary(pathPano0);
		std::vector<uint8_t> dataPano1 = assetsManager->GetResourcePackFileBinary(pathPano1);
		std::vector<uint8_t> dataPano2 = assetsManager->GetResourcePackFileBinary(pathPano2);
		std::vector<uint8_t> dataPano3 = assetsManager->GetResourcePackFileBinary(pathPano3);
		std::vector<uint8_t> dataPano4 = assetsManager->GetResourcePackFileBinary(pathPano4);
		std::vector<uint8_t> dataPano5 = assetsManager->GetResourcePackFileBinary(pathPano5);

		Skybox::CubeMapData cubeMapData{};
		cubeMapData.RightFaceData = dataPano0;
		cubeMapData.BackFaceData = dataPano1;
		cubeMapData.LeftFaceData = dataPano2;
		cubeMapData.FrontFaceData = dataPano3;
		cubeMapData.TopFaceData = dataPano4;
		cubeMapData.BottomFaceData = dataPano5;

		m_Skybox.ReloadTextures(cubeMapData);
	}

	void Gui::SubscribeToPanelsEvents()
	{
		m_EventHandles.push_back(GuiElement::RequestCursorStyleChange.Subscribe(
			[this](const CursorStyle& style) { Handle_CursorStyleChangeRequest(style); }));

		m_EventHandles.push_back(m_MainMenuPanel.RequestMenuNavigation.Subscribe(
			[this](const std::pair<const GuiElement*, eMenu>& request) { Handle_MenuNavigationRequest(request); }));

		m_EventHandles.push_back(m_DemoPanel.RequestMenuNavigation.Subscribe(
			[this](const std::pair<const GuiElement*, eMenu>& request) { Handle_MenuNavigationRequest(request); }));

		m_EventHandles.push_back(m_MainMenuPanel.RequestQuitGame.Subscribe([this](const GuiElement* sender)
																		   { Handle_QuitGameRequest(sender); }));

		m_EventHandles.push_back(m_PausePanel.RequestMenuNavigation.Subscribe(
			[this](const std::pair<const GuiElement*, eMenu>& request) { Handle_MenuNavigationRequest(request); }));

		m_EventHandles.push_back(m_PausePanel.RequestQuitToMainMenu.Subscribe(
			[this](const GuiElement* sender) { Handle_QuitToMainMenuRequest(sender); }));

		m_EventHandles.push_back(m_PausePanel.RequestBackToGame.Subscribe([this](const GuiElement* sender)
																		  { Handle_BackToGameRequest(sender); }));

		m_EventHandles.push_back(m_OptionsPanel.RequestMenuNavigation.Subscribe(
			[this](const std::pair<const GuiElement*, eMenu>& request) { Handle_MenuNavigationRequest(request); }));

		m_EventHandles.push_back(m_OptionsPanel.RequestBackNavigation.Subscribe([this](const GuiElement* sender)
																				{ Handle_BackRequest(sender); }));

		m_EventHandles.push_back(m_OptionsPanel.EvtUserSettingsChanged.Subscribe(
			[this](const UserSettingsChangedEventArgs& eventArgs) { Handle_UserSettingsChanged(eventArgs); }));

		m_EventHandles.push_back(m_ResourcePacksPanel.RequestBackNavigation.Subscribe([this](const GuiElement* sender)
																					  { Handle_BackRequest(sender); }));

		m_EventHandles.push_back(m_ResourcePacksPanel.RequestResourcePackChange.Subscribe(
			[this](const std::string& resourcePackName) { Handle_ResourcePackChangeRequest(resourcePackName); }));

		m_EventHandles.push_back(m_DemoScrollingPanel.RequestBackNavigation.Subscribe([this](const GuiElement* sender)
																					  { Handle_BackRequest(sender); }));

		m_EventHandles.push_back(m_SingleplayerPanel.EvtRequestBackNavigation.Subscribe(
			[this](const GuiElement* sender) { Handle_BackRequest(sender); }));

		m_EventHandles.push_back(m_SingleplayerPanel.EvtPlayWorld.Subscribe(
			[this](const WorldInfos& worldInfos) { RequestStartSingleplayerGame.Trigger(worldInfos); }));

		m_EventHandles.push_back(m_VideoSettingsPanel.EvtRequestBackNavigation.Subscribe(
			[this](const GuiElement* sender) { Handle_BackRequest(sender); }));

		m_EventHandles.push_back(m_VideoSettingsPanel.EvtUserSettingsChanged.Subscribe(
			[this](const UserSettingsChangedEventArgs& eventArgs) { Handle_UserSettingsChanged(eventArgs); }));

		m_EventHandles.push_back(m_ControlsPanel.EvtRequestBackNavigation.Subscribe([this](const GuiElement* sender)
																					{ Handle_BackRequest(sender); }));

		m_EventHandles.push_back(m_ControlsPanel.EvtUserSettingsChanged.Subscribe(
			[this](const UserSettingsChangedEventArgs& eventArgs) { Handle_UserSettingsChanged(eventArgs); }));

		m_EventHandles.push_back(m_ControlsPanel.EvtRequestMenuNavigation.Subscribe(
			[this](const std::pair<const GuiElement*, eMenu>& request) { Handle_MenuNavigationRequest(request); }));

		m_EventHandles.push_back(m_MouseSettingsPanel.EvtRequestBackNavigation.Subscribe(
			[this](const GuiElement* sender) { Handle_BackRequest(sender); }));

		m_EventHandles.push_back(m_MouseSettingsPanel.EvtUserSettingsChanged.Subscribe(
			[this](const UserSettingsChangedEventArgs& eventArgs) { Handle_UserSettingsChanged(eventArgs); }));

		m_EventHandles.push_back(m_KeyBindsPanel.EvtRequestBackNavigation.Subscribe([this](const GuiElement* sender)
																					{ Handle_BackRequest(sender); }));

		m_EventHandles.push_back(m_KeyBindsPanel.EvtUserSettingsChanged.Subscribe(
			[this](const UserSettingsChangedEventArgs& eventArgs) { Handle_UserSettingsChanged(eventArgs); }));
	}

	void Gui::Handle_MenuNavigationRequest(const std::pair<const GuiElement*, eMenu>& request)
	{
		const eMenu menu = request.second;

		SetActiveMenu(menu);

		if (menu == eMenu::Multiplayer)
		{
			std::string serverAddress = "127.0.0.1";
			uint16_t serverPort = 25565;
			MultiplayerGameStartInfo info{serverAddress, serverPort};
			RequestStartMultiplayerGame.Trigger(info);
		}
	}

	void Gui::Handle_QuitGameRequest(const GuiElement* sender)
	{
		(void) sender; // Unused parameter
		// Sends a SIGINT Signal to the Application.
		raise(SIGINT);
	}

	void Gui::Handle_CursorStyleChangeRequest(const CursorStyle& style)
	{
		RequestCursorStyleChange.Trigger(style);
	}

	void Gui::Handle_BackToGameRequest(const GuiElement* sender)
	{
		RequestBackToGame.Trigger(sender);
	}

	void Gui::Handle_QuitToMainMenuRequest(const GuiElement* sender)
	{
		RequestQuitToMainMenu.Trigger(sender);
	}

	void Gui::Handle_BackRequest(const GuiElement* sender)
	{
		(void) sender; // Unused parameter
		GoBackToPreviousMenu();
	}

	void Gui::Handle_ResourcePackChangeRequest(const std::string& resourcePackName)
	{
		std::cout << "Selected Resource Pack: " << resourcePackName << std::endl;

		RequestResourcePackChange.Trigger(resourcePackName);
	}

	void Gui::Handle_UserSettingsChanged(const UserSettingsChangedEventArgs& eventArgs)
	{
		UserSettingsChanged.Trigger(eventArgs);
	}

	void Gui::SetInputsSnapshot(std::shared_ptr<InputsSnapshot> inputsSnapshot)
	{
		GuiElement::SetInputsSnapshot(inputsSnapshot);
	}

	void Gui::Handle_FramebufferResized(const FramebufferState& framebufferState)
	{
		GuiElement::SetScreenSize(framebufferState.Width, framebufferState.Height);
		s_Camera.SetAspectRatio(static_cast<float>(framebufferState.Width) / framebufferState.Height);
	}

	void Gui::RenderDebugPanel()
	{
		ImGui::Begin("Gui");

		ImGui::Text("Active Menu: %s", GetMenuName(GetActiveMenu()).c_str());

		// ----- GuiElement Debug -----
		if (ImGui::CollapsingHeader("GuiElement", ImGuiTreeNodeFlags_DefaultOpen))
		{
			int guiScale = GetGuiScale();
			ImGui::SliderInt("Gui Scale", &guiScale, 1, 10);
			if (guiScale != GetGuiScale())
			{
				SetGuiScale(guiScale);
			}
		}

		ImGui::Separator();

		ImGui::SliderFloat("Skybox Rotation Period", &s_SkyboxRotationPeriod, 1.f, 500.f, "%.1f seconds");
		ImGui::SliderFloat("Camera FOV", &s_CameraFov, 30.f, 120.f, "%.1f degrees");

		ImGui::End();
	}

	void Gui::SetActiveMenu(eMenu menu, bool withHistory)
	{
		// Lock both mutexes to ensure thread safety when modifying the active menu and menu history.
		std::lock_guard lock(m_MutexState);

		// Store the previous menu before changing to the new one.
		eMenu previousMenu = m_ActiveMenu;
		m_ActiveMenu = menu;

		// Clear the menu history if we have navigated to the main menu.
		if (m_ActiveMenu == eMenu::MainMenu)
		{
			std::stack<eMenu>().swap(m_MenuHistory);
		}
		else
		{
			if (withHistory)
			{
				// If we are navigating to a different menu, add the previous menu to the history stack.
				if (previousMenu != eMenu::None && previousMenu != m_ActiveMenu)
				{
					// Avoid pushing the same menu multiple times.
					if (m_MenuHistory.empty() || m_MenuHistory.top() != previousMenu)
					{
						m_MenuHistory.push(previousMenu);
					}
				}
			}
		}

		// Cycle the main menu splash text when navigating to the main menu.
		if (m_ActiveMenu == eMenu::MainMenu)
		{
			m_MainMenuPanel.CycleSplashText();
		}

		if (m_ActiveMenu == eMenu::ResourcePacks)
		{
			m_ResourcePacksPanel.ScanResourcePacksFolder();
			std::string selectedResourcePackName = EngineContext::Get().Assets->GetCurrentResourcePack();
			m_ResourcePacksPanel.SetCurrentlySelectedResourcePack(selectedResourcePackName);
		}

		if (m_ActiveMenu == eMenu::Singleplayer)
		{
			m_SingleplayerPanel.RefreshWorldTiles();
		}
	}

	void Gui::GoBackToPreviousMenu()
	{
		eMenu targetMenu;

		{
			std::lock_guard lock(m_MutexState);

			if (m_MenuHistory.empty())
			{
				targetMenu = eMenu::MainMenu; // Default to main menu if history is empty
			}
			else
			{
				targetMenu = m_MenuHistory.top();
				m_MenuHistory.pop();
			}
		}

		SetActiveMenu(targetMenu, false);
	}

	eMenu Gui::GetActiveMenu() const
	{
		std::lock_guard lock(m_MutexState);
		return m_ActiveMenu;
	}

	void Gui::SetGuiScale(int scale)
	{
		GuiElement::SetGuiScale(scale);
	}

	int Gui::GetGuiScale() const
	{
		return GuiElement::GetGuiScale();
	}

	void Gui::SetIsInGame(bool isInGame)
	{
		m_IsInGame = isInGame;
	}

	void Gui::Initialize()
	{
		m_DemoPanel.Initialize();
		m_MainMenuPanel.Initialize();
		m_DemoScrollingPanel.Initialize();
		m_PausePanel.Initialize();
		m_OptionsPanel.Initialize();
		m_ResourcePacksPanel.Initialize();
		m_SingleplayerPanel.Initialize();
		m_VideoSettingsPanel.Initialize();
		m_ControlsPanel.Initialize();
		m_MouseSettingsPanel.Initialize();
		m_KeyBindsPanel.Initialize();

		ReloadSkyboxTextures();
	}

	void Gui::Render()
	{
		RenderDebugPanel();

		{
			// Reset the back button state if we have switched to a different menu since the last frame.
			// Prevents the Pause menu from immediately navigating back to the game when we open it while the back button is pressed.
			std::lock_guard lock(m_MutexState);
			if (m_ActiveMenu != m_MenuPreviousFrame)
			{
				GuiElement::s_IsBackPressed = false;
			}
		}

		const eMenu activeMenu = GetActiveMenu();
		switch (activeMenu)
		{
			case eMenu::DemoPanel:
				m_DemoPanel.Render();
				break;
			case eMenu::DemoScrollingPanel:
				m_DemoScrollingPanel.Render();
				break;
			case eMenu::MainMenu:
				m_MainMenuPanel.Render();
				break;
			case eMenu::Pause:
				m_PausePanel.Render();
				break;
			case eMenu::Options:
				m_OptionsPanel.Render();
				break;
			case eMenu::VideoSettings:
				m_VideoSettingsPanel.Render();
				break;
			case eMenu::ResourcePacks:
				m_ResourcePacksPanel.Render();
				break;
			case eMenu::Singleplayer:
				m_SingleplayerPanel.Render();
				break;
			case eMenu::Controls:
				m_ControlsPanel.Render();
				break;
			case eMenu::MouseSettings:
				m_MouseSettingsPanel.Render();
				break;
			case eMenu::KeyBinds:
				m_KeyBindsPanel.Render();
			default:
				break;
		}

		std::lock_guard lock(m_MutexState);
		m_MenuPreviousFrame = m_ActiveMenu;
	}

	void Gui::RenderBackground()
	{
		// Render the skybox if we are not in-game
		if (!m_IsInGame)
		{
			double time = glfwGetTime();

			// Convert time --> angle (radians)
			double angle = (time / s_SkyboxRotationPeriod) * glm::two_pi<double>();

			// Rotate around Y axis
			glm::vec3 facingDirection = glm::vec3(std::sin(angle), 0.0f, -std::cos(angle));

			// Sets the camera position and orientation
			s_Camera.SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
			s_Camera.SetFront(facingDirection);
			s_Camera.SetFovY(s_CameraFov);

			glm::mat4 view = s_Camera.GetViewMatrix();
			glm::mat4 projection = s_Camera.GetProjectionMatrix();
			m_Skybox.Render(view, projection);
		}
	}

	void Gui::Shutdown()
	{
		m_DemoPanel.Delete();
		m_MainMenuPanel.Delete();
		m_DemoScrollingPanel.Delete();
		m_PausePanel.Delete();
		m_OptionsPanel.Delete();
		m_ResourcePacksPanel.Delete();
		m_SingleplayerPanel.Delete();
		m_VideoSettingsPanel.Delete();
		m_ControlsPanel.Delete();
		m_MouseSettingsPanel.Delete();
		m_KeyBindsPanel.Delete();

		m_Skybox.Unload();
	}

	void Gui::ReloadTextures()
	{
		m_DemoPanel.ReloadTextures();
		m_MainMenuPanel.ReloadTextures();
		m_DemoScrollingPanel.ReloadTextures();
		m_PausePanel.ReloadTextures();
		m_OptionsPanel.ReloadTextures();
		m_ResourcePacksPanel.ReloadTextures();
		m_SingleplayerPanel.ReloadTextures();
		m_VideoSettingsPanel.ReloadTextures();
		m_ControlsPanel.ReloadTextures();
		m_MouseSettingsPanel.ReloadTextures();
		m_KeyBindsPanel.ReloadTextures();

		ReloadSkyboxTextures();
	}

} // namespace onion::voxel
