#include "MainMenuPanel.hpp"

#include "../../LayoutHelper.hpp"

#include <fstream>
#include <iostream>
#include <random>

namespace onion::voxel
{
	MainMenuPanel::MainMenuPanel(const std::string& name)
		: GuiElement(name), m_Title_Sprite("Title", m_SpriteTitlePath.string()), m_Singleplayer_Button("Singleplayer"),
		  m_Multiplayer_Button("Multiplayer"), m_DemoPanel_Button("Demo Panel"), m_Options_Button("Options"),
		  m_QuitGame_Button("Quit Game")
	{
		SubscribeToControlEvents();
		LoadSplashes();
		CycleSplashText();

		m_Singleplayer_Button.SetText("Singleplayer");
		m_Multiplayer_Button.SetText("Multiplayer");
		m_DemoPanel_Button.SetText("Demo Panel");
		m_Options_Button.SetText("Options...");
		m_QuitGame_Button.SetText("Quit Game");
	}

	void MainMenuPanel::Render()
	{
		// ---- Constants for Layout ----
		float glfwTime = (float) glfwGetTime();
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
		std::string versionText = "Voxel::Onion " + CLIENT_VERSION;
		float textX = s_ScreenHeight * 0.01f;
		float textY = s_ScreenHeight * 0.97f;
		float textHeight = s_ScreenHeight * (28.f / 1009.f);
		glm::vec3 textColor{1.f, 1.f, 1.f};
		float shadowOffset = textHeight / s_TextFont.GetGlyphSize().y;
		glm::vec3 shadowColor{0.246f, 0.246f, 0.246f};

		glm::vec2 textPos{textX, textY};
		glm::vec2 shadowOffsetVec{shadowOffset, shadowOffset};

		s_TextFont.RenderText(
			versionText, Font::eTextAlignment::Left, textPos + shadowOffsetVec, textHeight, shadowColor, 0.1f);
		s_TextFont.RenderText(versionText, Font::eTextAlignment::Left, textPos, textHeight, textColor, 0.2f);

		// ---- Render Copyright Text ----
		std::string copyrightText = "Uses Mojang's assets, DO NOT DISTRIBUTE.";
		glm::ivec2 textSize = s_TextFont.MeasureText(copyrightText, textHeight);
		float copyrightTextX = s_ScreenWidth - textSize.x - (s_ScreenWidth * 0.01f);

		glm::vec2 copyrightTextPos{copyrightTextX, textY};
		glm::vec2 copyrightShadowOffsetVec{shadowOffset, shadowOffset};

		s_TextFont.RenderText(copyrightText,
							  Font::eTextAlignment::Left,
							  copyrightTextPos + copyrightShadowOffsetVec,
							  textHeight,
							  shadowColor,
							  0.1f);
		s_TextFont.RenderText(copyrightText, Font::eTextAlignment::Left, copyrightTextPos, textHeight, textColor, 0.2f);

		// ---- Render Splash Text ----
		if (!m_Splashes.empty())
		{
			const std::string& splashText = m_Splashes[m_CurrentSplashIndex];
			float splashTextHeight = textHeight * m_SplashTextPulse.GetValue(glfwTime);
			glm::ivec2 splashTextSize = s_TextFont.MeasureText(splashText, splashTextHeight);
			float splashTextXratioCenter = 1420.f / 1920.f;
			float splashTextYratioCenter = 267.f / 1009.f;
			glm::vec3 splashTextColor{1.f, 1.f, 0.0f};
			glm::vec3 splashTextShadowColor = {0.246f, 0.246f, 0.0f};
			glm::vec2 textCenter = {s_ScreenWidth * splashTextXratioCenter, s_ScreenHeight * splashTextYratioCenter};
			float rotationDeg = -25.f;
			glm::vec2 shadowOffsetSplash = glm::vec2(splashTextHeight / s_TextFont.GetGlyphSize().y);

			s_TextFont.RenderText(splashText,
								  Font::eTextAlignment::Center,
								  textCenter + shadowOffsetSplash,
								  splashTextHeight,
								  splashTextShadowColor,
								  0.1f,
								  rotationDeg);

			s_TextFont.RenderText(splashText,
								  Font::eTextAlignment::Center,
								  textCenter,
								  splashTextHeight,
								  splashTextColor,
								  0.2f,
								  rotationDeg);
		}
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

	void MainMenuPanel::CycleSplashText()
	{
		if (m_Splashes.empty())
			return;

		// Select a random splash text from the list
		static std::random_device rd;  // Seed source
		static std::mt19937 gen(rd()); // Mersenne Twister engine
		std::uniform_int_distribution<int> dist(0, m_Splashes.size() - 1);

		m_CurrentSplashIndex = dist(gen);
	}

	void MainMenuPanel::LoadSplashes()
	{
		m_Splashes.clear();
		std::ifstream file(m_SplashScreenTextPath);

		if (!file.is_open())
		{
			std::cerr << "Failed to open splashes file: " << m_SplashScreenTextPath << std::endl;
			return;
		}

		std::string line;
		while (std::getline(file, line))
		{
			if (!line.empty())
			{
				m_Splashes.push_back(line);
			}
		}

		file.close();
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
