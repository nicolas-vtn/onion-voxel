#include "Gui.hpp"

#include <imgui.h>

#include <csignal>

namespace onion::voxel
{
	void Gui::StaticInitialize()
	{
		GuiElement::Load();
	}

	void Gui::StaticShutdown()
	{
		GuiElement::Unload();
	}

	Gui::Gui()
		: m_DemoPanel("DemoPanel"), m_MainMenuPanel("MainMenuPanel"), m_PausePanel("PausePanel"),
		  m_OptionsPanel("OptionsPanel"), m_ResourcePacksPanel("ResourcePacksPanel")
	{
		SubscribeToPannelsEvents();
	}

	Gui::~Gui()
	{
		m_EventHandles.clear();
	}

	void Gui::SubscribeToPannelsEvents()
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

		m_EventHandles.push_back(m_ResourcePacksPanel.RequestBackNavigation.Subscribe([this](const GuiElement* sender)
																					  { Handle_BackRequest(sender); }));
	}

	void Gui::Handle_MenuNavigationRequest(const std::pair<const GuiElement*, eMenu>& request)
	{
		const eMenu menu = request.second;

		SetActiveMenu(menu);

		// WIP : Temporary trigger
		if (menu == eMenu::Singleplayer)
		{
			RequestStartSingleplayerGame.Trigger(GetAssetsPath() / "worlds" / "demo_map");
		}
	}

	void Gui::Handle_QuitGameRequest(const GuiElement* sender)
	{
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
		GoBackToPreviousMenu();
	}

	void Gui::SetInputsSnapshot(std::shared_ptr<InputsSnapshot> inputsSnapshot)
	{
		GuiElement::SetInputsSnapshot(inputsSnapshot);
	}

	void Gui::SetScreenSize(int screenWidth, int screenHeight)
	{
		GuiElement::SetScreenSize(screenWidth, screenHeight);
	}

	void Gui::RenderDebugPanel()
	{
		ImGui::Begin("Gui");

		ImGui::Text("Active Menu: %s",
					[this]()
					{
						switch (GetActiveMenu())
						{
							case eMenu::DemoPanel:
								return "Demo Panel";
							case eMenu::MainMenu:
								return "Main Menu";
							case eMenu::Singleplayer:
								return "Singleplayer";
							case eMenu::Multiplayer:
								return "Multiplayer";
							case eMenu::Options:
								return "Settings";
							case eMenu::Gameplay:
								return "Gameplay";
							case eMenu::Pause:
								return "Pause Menu";
							default:
								return "None";
						}
					}());

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

	void Gui::Initialize()
	{
		m_DemoPanel.Initialize();
		m_MainMenuPanel.Initialize();
		m_PausePanel.Initialize();
		m_OptionsPanel.Initialize();
		m_ResourcePacksPanel.Initialize();
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

		switch (GetActiveMenu())
		{
			case eMenu::DemoPanel:
				m_DemoPanel.Render();
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
			case eMenu::ResourcePacks:
				m_ResourcePacksPanel.Render();
				break;
			default:
				break;
		}

		std::lock_guard lock(m_MutexState);
		m_MenuPreviousFrame = m_ActiveMenu;
	}

	void Gui::Shutdown()
	{
		m_DemoPanel.Delete();
		m_MainMenuPanel.Delete();
		m_PausePanel.Delete();
		m_OptionsPanel.Delete();
		m_ResourcePacksPanel.Delete();
	}

} // namespace onion::voxel
