#include "MainMenuPanel.hpp"

#include "../../LayoutHelper.hpp"

namespace onion::voxel
{
	MainMenuPanel::MainMenuPanel(const std::string& name)
		: GuiElement(name), m_Title_Sprite("Title", m_SpriteTitlePath.string()), m_Singleplayer_Button("Singleplayer"),
		  m_Multiplayer_Button("Multiplayer"), m_DemoPanel_Button("Demo Panel"), m_Options_Button("Options"),
		  m_QuitGame_Button("Quit Game")
	{
		SubscribeToControlEvents();

		m_Singleplayer_Button.SetText("Singleplayer");
		m_Multiplayer_Button.SetText("Multiplayer");
		m_DemoPanel_Button.SetText("Demo Panel");
		m_Options_Button.SetText("Options...");
		m_QuitGame_Button.SetText("Quit Game");
	}

	void MainMenuPanel::Render()
	{
		glm::vec2 buttonSizeRatio{0.415f, 0.08f};
		glm::vec2 buttonSize{buttonSizeRatio.x * s_ScreenWidth, buttonSizeRatio.y * s_ScreenHeight};
		float buttonYSpacingRatio = 94.f / 1009.f;
		float firstButtonYPosRatio = 486.f / 1009.f;

		// ---- Render Title Sprite ----
		float spriteXScaleFacor = 0.5f;
		float spriteXRatio = 0.5f;
		float spriteYRatio = 0.2f;
		float aspectRatio = (float) m_Title_Sprite.GetTextureHeight() / m_Title_Sprite.GetTextureWidth();

		const glm::vec2 spritePos{s_ScreenWidth * spriteXRatio, s_ScreenHeight * spriteYRatio};
		float spriteSizeX = s_ScreenWidth * spriteXScaleFacor;
		float spriteSizeY = spriteSizeX * aspectRatio;
		const glm::vec2 spriteSize{spriteSizeX, spriteSizeY};

		m_Title_Sprite.SetPosition(spritePos);
		m_Title_Sprite.SetSize(spriteSize);
		m_Title_Sprite.Render();

		// ---- Render Singleplayer Button ----
		glm::vec2 buttonPos{s_ScreenWidth * 0.5f, s_ScreenHeight * firstButtonYPosRatio};
		m_Singleplayer_Button.SetPosition(buttonPos);
		m_Singleplayer_Button.SetSize(buttonSize);
		m_Singleplayer_Button.Render();

		// ---- Render Multiplayer Button ----
		buttonPos = {s_ScreenWidth * 0.5f, s_ScreenHeight * (firstButtonYPosRatio + buttonYSpacingRatio)};
		m_Multiplayer_Button.SetPosition(buttonPos);
		m_Multiplayer_Button.SetSize(buttonSize);
		m_Multiplayer_Button.Render();

		// ---- Render Demo Panel Button ----
		buttonPos = {s_ScreenWidth * 0.5f, s_ScreenHeight * (firstButtonYPosRatio + buttonYSpacingRatio * 2)};
		m_DemoPanel_Button.SetPosition(buttonPos);
		m_DemoPanel_Button.SetSize(buttonSize);
		m_DemoPanel_Button.Render();

		// ---- Build Layout for Options Button and Quit Game Button ----
		float tableWidth = buttonSize.x;
		float tableHeight = buttonSize.y;
		float horizontalSpacing = 0.02f * tableWidth;
		float verticalSpacing = 0;

		float tableButtonYPosRatio = 780.f / 1009.f;

		glm::ivec2 topLeftOfTable{s_ScreenWidth * 0.5 - (tableWidth / 2), s_ScreenHeight * tableButtonYPosRatio};

		TableLayout tableLayout = LayoutHelper::CreateTableLayout(
			1, 2, glm::ivec2(tableWidth, tableHeight), horizontalSpacing, verticalSpacing);
		const glm::ivec2 cellSize = tableLayout.GetCellSize();

		// ---- Render Options Button ----
		glm::ivec2 relativeButtonPos = tableLayout.GetElementPosition(0, 0);
		m_Options_Button.SetPosition(topLeftOfTable + relativeButtonPos);
		m_Options_Button.SetSize(cellSize);
		m_Options_Button.Render();

		// ---- Render Quit Game Button ----
		relativeButtonPos = tableLayout.GetElementPosition(0, 1);
		m_QuitGame_Button.SetPosition(topLeftOfTable + relativeButtonPos);
		m_QuitGame_Button.SetSize(cellSize);
		m_QuitGame_Button.Render();

		// ---- Render Version Text ----
		std::string versionText = "Voxel::Onion " + m_GameVersion;
		float textX = s_ScreenHeight * 0.01f;
		float textY = s_ScreenHeight * 0.96f;
		float textHeight = s_ScreenHeight * (28.f / 1009.f);
		glm::vec3 textColor = {1.f, 1.f, 1.f};
		float shadowOffset = textHeight / s_TextFont.GetGlyphSize().y;
		glm::vec3 shadowColor = {0.246f, 0.246f, 0.246f};

		s_TextFont.RenderText(versionText, textX + shadowOffset, textY + shadowOffset, textHeight, shadowColor);
		s_TextFont.RenderText(versionText, textX, textY, textHeight, textColor);

		// ---- Render Copyright Text ----
		std::string copyrightText = "Uses Mojang's assets, DO NOT DISTRIBUTE.";
		glm::ivec2 textSize = s_TextFont.MeasureText(copyrightText, textHeight);
		float copyrightTextX = s_ScreenWidth - textSize.x - (s_ScreenWidth * 0.01f);

		s_TextFont.RenderText(
			copyrightText, copyrightTextX + shadowOffset, textY + shadowOffset, textHeight, shadowColor);
		s_TextFont.RenderText(copyrightText, copyrightTextX, textY, textHeight, textColor);
	}

	void MainMenuPanel::Initialize()
	{
		m_Title_Sprite.Initialize();

		m_Singleplayer_Button.Initialize();
		m_Multiplayer_Button.Initialize();
		m_DemoPanel_Button.Initialize();
		m_Options_Button.Initialize();
		m_QuitGame_Button.Initialize();

		SetInitState(true);
	}

	void MainMenuPanel::Delete()
	{
		m_Title_Sprite.Delete();

		m_Singleplayer_Button.Delete();
		m_Multiplayer_Button.Delete();
		m_DemoPanel_Button.Delete();
		m_Options_Button.Delete();
		m_QuitGame_Button.Delete();

		SetDeletedState(true);
	}

	void MainMenuPanel::SetGameVersion(const std::string& version)
	{
		m_GameVersion = version;
	}

	void MainMenuPanel::SubscribeToControlEvents()
	{
		m_EventHandles.push_back(m_Singleplayer_Button.OnClick.Subscribe(
			[this](const Button& sender) { RequestMenuNavigation.Trigger(eMenu::Singleplayer); }));

		m_EventHandles.push_back(m_Multiplayer_Button.OnClick.Subscribe(
			[this](const Button& sender) { RequestMenuNavigation.Trigger(eMenu::Multiplayer); }));

		m_EventHandles.push_back(m_DemoPanel_Button.OnClick.Subscribe(
			[this](const Button& sender) { RequestMenuNavigation.Trigger(eMenu::DemoPanel); }));

		m_EventHandles.push_back(m_Options_Button.OnClick.Subscribe(
			[this](const Button& sender) { RequestMenuNavigation.Trigger(eMenu::Settings); }));

		m_EventHandles.push_back(
			m_QuitGame_Button.OnClick.Subscribe([this](const Button& sender) { RequestQuitGame.Trigger(this); }));
	}

	void MainMenuPanel::Handle_Singleplayer_Click(const Button& sender) {}

	void MainMenuPanel::Handle_Multiplayer_Click(const Button& sender) {}

	void MainMenuPanel::Handle_DemoButton_Click(const Button& sender) {}

	void MainMenuPanel::Handle_Options_Click(const Button& sender) {}

	void MainMenuPanel::Handle_QuitGame_Click(const Button& sender) {}
}; // namespace onion::voxel
