#include "PausePanel.hpp"

#include "../../LayoutHelper.hpp"

namespace onion::voxel
{
	PausePanel::PausePanel(const std::string& name)
		: GuiElement(name), m_Title_Label("PauseTitle_Label"), m_BackToGame_Button("BackToGame_Button"),
		  m_Options_Button("Options_Button"), m_MainMenu_Button("MainMenu_Button"),
		  m_Advancements_Button("Advancements_Button"), m_Statistics_Button("Statistics_Button"),
		  m_GiveFeedback_Button("GiveFeedback_Button"), m_ReportBugs_Button("ReportBugs_Button"),
		  m_OpenToLan_Button("OpenToLan_Button")
	{
		SubscribeToControlEvents();

		m_Title_Label.SetText("Game Menu");
		m_Title_Label.SetTextAlignment(Font::eTextAlignment::Center);

		m_BackToGame_Button.SetText("Back to Game");

		m_Advancements_Button.SetText("Advancements");
		m_Advancements_Button.SetEnabled(false);

		m_Statistics_Button.SetText("Statistics");
		m_Statistics_Button.SetEnabled(false);

		m_GiveFeedback_Button.SetText("Give Feedback");
		m_GiveFeedback_Button.SetEnabled(false);

		m_ReportBugs_Button.SetText("Report Bugs");
		m_ReportBugs_Button.SetEnabled(false);

		m_Options_Button.SetText("Options...");

		m_OpenToLan_Button.SetText("Open to LAN");
		m_OpenToLan_Button.SetEnabled(false);

		m_MainMenu_Button.SetText("Save and Quit to Title");
	}

	PausePanel::~PausePanel()
	{
		m_EventHandles.clear();
	}

	void PausePanel::Render()
	{
		if (s_IsBackPressed)
		{
			RequestBackToGame.Trigger(this);
			return;
		}

		// ---- Constants for Layout ----
		glm::vec2 buttonSizeRatio{0.415f, 0.08f};
		glm::vec2 buttonSize{buttonSizeRatio.x * s_ScreenWidth, buttonSizeRatio.y * s_ScreenHeight};
		float buttonYSpacingRatio = 94.f / 1009.f;
		float firstButtonYPosRatio = 486.f / 1009.f;
		float middleX = s_ScreenWidth * 0.5f;

		// ---- Render Menu Title ----
		constexpr float menuYOffsetRatio = (200.f - 23.f) / 1009.f;
		glm::vec2 textPosition = {s_ScreenWidth / 2, s_ScreenHeight * menuYOffsetRatio};
		float textHeight = s_ScreenHeight * (30.f / 1009.f);

		m_Title_Label.SetPosition(textPosition);
		m_Title_Label.SetTextHeight(textHeight);
		m_Title_Label.Render();

		// ---- Prepare Layout for Buttons ----
		float tableXratio = 816.f / 1920.f;
		float tableYratio = 462.f / 1009.f;
		float tableWidth = s_ScreenWidth * tableXratio;
		float tableHeight = s_ScreenHeight * tableYratio;
		float horizontalSpacingRatio = 33.f / 1920.f;
		float horizontalSpacing = s_ScreenWidth * horizontalSpacingRatio;
		float verticalSpacingRatio = 17.f / 1009.f;
		float verticalSpacing = s_ScreenHeight * verticalSpacingRatio;

		float tableButtonYPosRatio = 300.f / 1009.f;

		glm::ivec2 topLeftOfTable{s_ScreenWidth * 0.5 - (tableWidth / 2), s_ScreenHeight * tableButtonYPosRatio};

		TableLayout tableLayout = LayoutHelper::CreateTableLayout(
			5, 2, glm::ivec2(tableWidth, tableHeight), horizontalSpacing, verticalSpacing);

		const glm::ivec2 cellSize = tableLayout.GetCellSize();

		// ---- Render Options Button ----
		glm::ivec2 relativeButtonPos = tableLayout.GetElementPosition(0, 0);
		m_BackToGame_Button.SetPosition({middleX, (topLeftOfTable + relativeButtonPos).y});
		m_BackToGame_Button.SetSize({tableWidth, cellSize.y}); // Full Table Width
		m_BackToGame_Button.Render();

		// ---- Render Advancements Button ----
		relativeButtonPos = tableLayout.GetElementPosition(1, 0);
		m_Advancements_Button.SetPosition(topLeftOfTable + relativeButtonPos);
		m_Advancements_Button.SetSize(cellSize);
		m_Advancements_Button.Render();

		// ---- Render Statistics Button ----
		relativeButtonPos = tableLayout.GetElementPosition(1, 1);
		m_Statistics_Button.SetPosition(topLeftOfTable + relativeButtonPos);
		m_Statistics_Button.SetSize(cellSize);
		m_Statistics_Button.Render();

		// ---- Render Give Feedback Button ----
		relativeButtonPos = tableLayout.GetElementPosition(2, 0);
		m_GiveFeedback_Button.SetPosition(topLeftOfTable + relativeButtonPos);
		m_GiveFeedback_Button.SetSize(cellSize);
		m_GiveFeedback_Button.Render();

		// ---- Render Report Bugs Button ----
		relativeButtonPos = tableLayout.GetElementPosition(2, 1);
		m_ReportBugs_Button.SetPosition(topLeftOfTable + relativeButtonPos);
		m_ReportBugs_Button.SetSize(cellSize);
		m_ReportBugs_Button.Render();

		// ---- Render Options Button ----
		relativeButtonPos = tableLayout.GetElementPosition(3, 0);
		m_Options_Button.SetPosition(topLeftOfTable + relativeButtonPos);
		m_Options_Button.SetSize(cellSize);
		m_Options_Button.Render();

		// ---- Render Open to LAN Button ----
		relativeButtonPos = tableLayout.GetElementPosition(3, 1);
		m_OpenToLan_Button.SetPosition(topLeftOfTable + relativeButtonPos);
		m_OpenToLan_Button.SetSize(cellSize);
		m_OpenToLan_Button.Render();

		// ---- Render Main Menu Button ----
		relativeButtonPos = tableLayout.GetElementPosition(4, 0);
		m_MainMenu_Button.SetPosition({middleX, (topLeftOfTable + relativeButtonPos).y});
		m_MainMenu_Button.SetSize({tableWidth, cellSize.y}); // Full Table Width
		m_MainMenu_Button.Render();
	}

	void PausePanel::Initialize()
	{
		m_Title_Label.Initialize();
		m_BackToGame_Button.Initialize();
		m_Options_Button.Initialize();
		m_MainMenu_Button.Initialize();
		m_Advancements_Button.Initialize();
		m_Statistics_Button.Initialize();
		m_GiveFeedback_Button.Initialize();
		m_ReportBugs_Button.Initialize();
		m_OpenToLan_Button.Initialize();

		SetInitState(true);
	}

	void PausePanel::Delete()
	{
		m_Title_Label.Delete();
		m_BackToGame_Button.Delete();
		m_Options_Button.Delete();
		m_MainMenu_Button.Delete();
		m_Advancements_Button.Delete();
		m_Statistics_Button.Delete();
		m_GiveFeedback_Button.Delete();
		m_ReportBugs_Button.Delete();
		m_OpenToLan_Button.Delete();

		SetDeletedState(true);
	}

	void PausePanel::SubscribeToControlEvents()
	{
		m_EventHandles.push_back(
			m_BackToGame_Button.OnClick.Subscribe([this](const Button& sender) { Handle_BackToGame_Click(sender); }));

		m_EventHandles.push_back(
			m_Options_Button.OnClick.Subscribe([this](const Button& sender) { Handle_Options_Click(sender); }));

		m_EventHandles.push_back(
			m_MainMenu_Button.OnClick.Subscribe([this](const Button& sender) { Handle_MainMenu_Click(sender); }));
	}

	void PausePanel::Handle_BackToGame_Click(const Button& sender)
	{
		RequestBackToGame.Trigger(this);
	}

	void PausePanel::Handle_Options_Click(const Button& sender)
	{
		RequestMenuNavigation.Trigger({this, eMenu::Options});
	}

	void PausePanel::Handle_MainMenu_Click(const Button& sender)
	{
		RequestQuitToMainMenu.Trigger({this});
	}

} // namespace onion::voxel
