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

	Gui::Gui() : m_DemoPanel("DemoPanel"), m_MainMenuPanel("MainMenuPanel"), m_PausePanel("PausePanel")
	{
		SubscribeToPannelsEvents();
	}

	Gui::~Gui() {}

	void Gui::SubscribeToPannelsEvents()
	{
		m_EventHandles.push_back(GuiElement::RequestCursorStyleChange.Subscribe(
			[this](const CursorStyle& style) { Handle_CursorStyleChangeRequest(style); }));

		m_EventHandles.push_back(m_MainMenuPanel.RequestMenuNavigation.Subscribe(
			[this](const eMenu& menu) { Handle_MenuNavigationRequest(menu); }));

		m_EventHandles.push_back(m_DemoPanel.RequestMenuNavigation.Subscribe([this](const eMenu& menu)
																			 { Handle_MenuNavigationRequest(menu); }));

		m_EventHandles.push_back(m_MainMenuPanel.RequestQuitGame.Subscribe([this](const GuiElement* sender)
																		   { Handle_QuitGameRequest(sender); }));

		m_EventHandles.push_back(m_PausePanel.RequestMenuNavigation.Subscribe([this](const eMenu& menu)
																			  { Handle_MenuNavigationRequest(menu); }));

		m_EventHandles.push_back(
			m_PausePanel.RequestQuitToMainMenu.Subscribe([this](bool quit) { Handle_QuitToMainMenuRequest(quit); }));

		m_EventHandles.push_back(
			m_PausePanel.RequestGameResume.Subscribe([this](bool resume) { Handle_GameResumeRequest(resume); }));
	}

	void Gui::Handle_MenuNavigationRequest(const eMenu& menu)
	{
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

	void Gui::Handle_GameResumeRequest(bool resume)
	{
		RequestGameResume.Trigger(resume);
	}

	void Gui::Handle_QuitToMainMenuRequest(bool quit)
	{
		RequestQuitToMainMenu.Trigger(quit);
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
							case eMenu::Settings:
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

	void Gui::SetActiveMenu(eMenu menu)
	{
		std::lock_guard lock(m_MutexState);
		m_ActiveMenu = menu;

		if (m_ActiveMenu == eMenu::MainMenu)
		{
			m_MainMenuPanel.CycleSplashText();
		}
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
	}

	void Gui::Render()
	{
		RenderDebugPanel();

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
			default:
				break;
		}
	}

	void Gui::Shutdown()
	{
		m_DemoPanel.Delete();
		m_MainMenuPanel.Delete();
		m_PausePanel.Delete();
	}

} // namespace onion::voxel
