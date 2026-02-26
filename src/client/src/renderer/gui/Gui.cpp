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

	void Gui::SetInputsSnapshot(std::shared_ptr<InputsSnapshot> inputsSnapshot)
	{
		GuiElement::SetInputsSnapshot(inputsSnapshot);
	}

	void Gui::SetScreenSize(int screenWidth, int screenHeight)
	{
		GuiElement::SetScreenSize(screenWidth, screenHeight);
	}

	Gui::Gui() {}

	Gui::~Gui() {}

	void Gui::SetActiveMenu(eMenu menu)
	{
		std::lock_guard lock(m_MutexState);
		m_ActiveMenu = menu;
	}

	Gui::eMenu Gui::GetActiveMenu() const
	{
		std::lock_guard lock(m_MutexState);
		return m_ActiveMenu;
	}

	void Gui::Initialize()
	{
		m_DemoPanel = std::make_unique<DemoPanel>("DemoPanel");
		m_DemoPanel->Initialize();
	}

	void Gui::Render()
	{
		switch (GetActiveMenu())
		{
			case eMenu::DemoPanel:
				m_DemoPanel->Render();
				break;
			default:
				break;
		}
	}

	void Gui::Shutdown()
	{
		m_DemoPanel->Delete();
		m_DemoPanel.reset();
	}

} // namespace onion::voxel
