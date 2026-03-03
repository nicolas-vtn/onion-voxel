#include "Gui.hpp"

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

	Gui::Gui() : m_DemoPanel("DemoPanel"), m_MainMenuPanel("MainMenuPanel")
	{
		SubscribeToPannelsEvents();
	}

	Gui::~Gui() {}

	void Gui::SubscribeToPannelsEvents()
	{
		m_EventHandles.push_back(m_MainMenuPanel.RequestMenuNavigation.Subscribe(
			[this](const eMenu& menu) { Handle_MenuNavigationRequest(menu); }));

		m_EventHandles.push_back(m_DemoPanel.RequestMenuNavigation.Subscribe([this](const eMenu& menu)
																			 { Handle_MenuNavigationRequest(menu); }));
	}

	void Gui::Handle_MenuNavigationRequest(const eMenu& menu)
	{
		SetActiveMenu(menu);
	}

	void Gui::SetInputsSnapshot(std::shared_ptr<InputsSnapshot> inputsSnapshot)
	{
		GuiElement::SetInputsSnapshot(inputsSnapshot);
	}

	void Gui::SetScreenSize(int screenWidth, int screenHeight)
	{
		GuiElement::SetScreenSize(screenWidth, screenHeight);
	}

	void Gui::SetGameVersion(const std::string& version)
	{
		m_MainMenuPanel.SetGameVersion(version);
	}

	void Gui::SetActiveMenu(eMenu menu)
	{
		std::lock_guard lock(m_MutexState);
		m_ActiveMenu = menu;
	}

	eMenu Gui::GetActiveMenu() const
	{
		std::lock_guard lock(m_MutexState);
		return m_ActiveMenu;
	}

	void Gui::Initialize()
	{
		m_DemoPanel.Initialize();
		m_MainMenuPanel.Initialize();
	}

	void Gui::Render()
	{
		switch (GetActiveMenu())
		{
			case eMenu::DemoPanel:
				m_DemoPanel.Render();
				break;
			case eMenu::MainMenu:
				m_MainMenuPanel.Render();
			default:
				break;
		}
	}

	void Gui::Shutdown()
	{
		m_DemoPanel.Delete();
		m_MainMenuPanel.Delete();
	}

} // namespace onion::voxel
