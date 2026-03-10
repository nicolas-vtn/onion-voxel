#include "PausePanel.hpp"

namespace onion::voxel
{
	PausePanel::PausePanel(const std::string& name)
		: GuiElement(name), m_Resume_Button("Resume_Button"), m_Options_Button("Options_Button"),
		  m_MainMenu_Button("MainMenu_Button")
	{
		SubscribeToControlEvents();

		m_Resume_Button.SetText("Resume");
		m_Options_Button.SetText("Options...");
		m_MainMenu_Button.SetText("Quit to Main Menu");
	}

	PausePanel::~PausePanel()
	{
		m_EventHandles.clear();
	}

	void PausePanel::Render()
	{
		// ---- Constants for Layout ----
		glm::vec2 buttonSizeRatio{0.415f, 0.08f};
		glm::vec2 buttonSize{buttonSizeRatio.x * s_ScreenWidth, buttonSizeRatio.y * s_ScreenHeight};
		float buttonYSpacingRatio = 94.f / 1009.f;
		float firstButtonYPosRatio = 486.f / 1009.f;

		// ---- Render Singleplayer Button ----
		glm::vec2 buttonPos{s_ScreenWidth * 0.5f, s_ScreenHeight * firstButtonYPosRatio};
		m_Resume_Button.SetPosition(buttonPos);
		m_Resume_Button.SetSize(buttonSize);
		m_Resume_Button.Render();

		// ---- Render Options Button ----
		buttonPos = {s_ScreenWidth * 0.5f, s_ScreenHeight * (firstButtonYPosRatio + buttonYSpacingRatio)};
		m_Options_Button.SetPosition(buttonPos);
		m_Options_Button.SetSize(buttonSize);
		m_Options_Button.Render();

		// ---- Render Multiplayer Button ----
		buttonPos = {s_ScreenWidth * 0.5f, s_ScreenHeight * (firstButtonYPosRatio + 3 * buttonYSpacingRatio)};
		m_MainMenu_Button.SetPosition(buttonPos);
		m_MainMenu_Button.SetSize(buttonSize);
		m_MainMenu_Button.Render();
	}

	void PausePanel::Initialize()
	{
		m_Resume_Button.Initialize();
		m_Options_Button.Initialize();
		m_MainMenu_Button.Initialize();

		SetInitState(true);
	}

	void PausePanel::Delete()
	{
		m_Resume_Button.Delete();
		m_Options_Button.Delete();
		m_MainMenu_Button.Delete();

		SetDeletedState(true);
	}

	void PausePanel::SubscribeToControlEvents()
	{
		m_EventHandles.push_back(
			m_Resume_Button.OnClick.Subscribe([this](const Button& sender) { Handle_Resume_Click(sender); }));

		m_EventHandles.push_back(
			m_Options_Button.OnClick.Subscribe([this](const Button& sender) { Handle_Options_Click(sender); }));

		m_EventHandles.push_back(
			m_MainMenu_Button.OnClick.Subscribe([this](const Button& sender) { Handle_MainMenu_Click(sender); }));
	}

	void PausePanel::Handle_Resume_Click(const Button& sender)
	{
		RequestGameResume.Trigger(true);
	}

	void PausePanel::Handle_Options_Click(const Button& sender)
	{
		RequestMenuNavigation.Trigger(eMenu::Settings);
	}

	void PausePanel::Handle_MainMenu_Click(const Button& sender)
	{
		RequestQuitToMainMenu.Trigger(true);
	}

} // namespace onion::voxel
