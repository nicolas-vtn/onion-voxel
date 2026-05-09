#include "MainMenuPanel.hpp"

#include <renderer/gui/LayoutHelper.hpp>

#include "version.hpp"

#include <fstream>
#include <iostream>
#include <random>

namespace onion::voxel
{
	MainMenuPanel::MainMenuPanel(const std::string& name)
		: GuiElement(name), m_Title_Sprite("Title", s_SpriteTitlePath, Sprite::eOrigin::Asset),
		  m_Singleplayer_Button("Singleplayer"), m_Multiplayer_Button("Multiplayer"), m_DemoPanel_Button("Demo Panel"),
		  m_Options_Button("Options"), m_QuitGame_Button("Quit Game"), m_SplashText_Label("Splash Text"),
		  m_Version_Label("Version"), m_Copyright_Label("Copyright")
	{
		SubscribeToControlEvents();

		m_Singleplayer_Button.SetText("Singleplayer");

		m_Multiplayer_Button.SetText("Multiplayer");

		m_DemoPanel_Button.SetText("Demo Panel");

		m_Options_Button.SetText("Options...");

		m_QuitGame_Button.SetText("Quit Game");

		m_SplashText_Label.SetCustomTextColor({1.f, 1.f, 0.0f, 1.f});
		m_SplashText_Label.SetZOffset(0.9f);
		m_SplashText_Label.SetRotationDegrees(-25.f);
		m_SplashText_Label.SetTextAlignment(Font::eTextAlignment::Center);

		std::string versionText = "Onion::Voxel " + std::string(GetProjectVersion());
		m_Version_Label.SetText(versionText);
		m_Version_Label.SetTextAlignment(Font::eTextAlignment::Left);

		m_Copyright_Label.SetText("Uses Mojang's assets, DO NOT DISTRIBUTE.");
		m_Copyright_Label.SetTextAlignment(Font::eTextAlignment::Right);
	}

	MainMenuPanel::~MainMenuPanel()
	{
		m_EventHandles.clear();
	}

	void MainMenuPanel::Render()
	{
		float glfwTime = (float) glfwGetTime();

		// ---- Title Sprite ----
		// Centered horizontally at x=160, vertically at y=48 (logical).
		// Width is 176 logical px; height preserves texture aspect ratio.
		constexpr float kTitleLogicalWidth = 280.f;
		float spriteAspectRatio = (float) m_Title_Sprite.GetTextureHeight() / m_Title_Sprite.GetTextureWidth();
		float spriteSizeX = Ls(kTitleLogicalWidth);
		float spriteSizeY = spriteSizeX * spriteAspectRatio;

		m_Title_Sprite.SetPosition(L(160.f, 48.f));
		m_Title_Sprite.SetSize({spriteSizeX, spriteSizeY});
		m_Title_Sprite.Render();

		// ---- Buttons — 200×20 logical px, stacked from y=116 with 24 spacing ----
		constexpr float kButtonW   = 200.f;
		constexpr float kButtonH   = 20.f;
		constexpr float kFirstBtnY = 116.f;
		constexpr float kSpacing   = 24.f;
		glm::vec2 buttonSize{Ls(kButtonW), Ls(kButtonH)};

		m_Singleplayer_Button.SetPosition(L(160.f, kFirstBtnY));
		m_Singleplayer_Button.SetSize(buttonSize);
		m_Singleplayer_Button.Render();

		m_Multiplayer_Button.SetPosition(L(160.f, kFirstBtnY + kSpacing));
		m_Multiplayer_Button.SetSize(buttonSize);
		m_Multiplayer_Button.Render();

		m_DemoPanel_Button.SetPosition(L(160.f, kFirstBtnY + kSpacing * 2.f));
		m_DemoPanel_Button.SetSize(buttonSize);
		m_DemoPanel_Button.Render();

		// ---- Options + Quit Game — side-by-side, same total width ----
		constexpr float kTableBtnY = kFirstBtnY + kSpacing * 3.f + 4.f;
		float tableWidth = Ls(kButtonW);
		float tableHeight = Ls(kButtonH);
		float horizontalSpacing = Ls(2.f);

		glm::ivec2 topLeftOfTable{(int) Lx(160.f - kButtonW / 2.f), (int) Ly(kTableBtnY)};

		TableLayout tableLayout = LayoutHelper::CreateTableLayout(
			1, 2, glm::ivec2(tableWidth, tableHeight), (int) horizontalSpacing, 0);
		const glm::ivec2 cellSize = tableLayout.GetCellSize();

		m_Options_Button.SetPosition(topLeftOfTable + tableLayout.GetElementPosition(0, 0));
		m_Options_Button.SetSize(cellSize);
		m_Options_Button.Render();

		m_QuitGame_Button.SetPosition(topLeftOfTable + tableLayout.GetElementPosition(0, 1));
		m_QuitGame_Button.SetSize(cellSize);
		m_QuitGame_Button.Render();

		// ---- Bottom labels ----
		// Anchored to physical screen edges with a 1-scale-unit margin, independent of canvas.
		float margin = (float) s_ActiveGuiScale.load();
		float labelY = (float) s_ScreenHeight - margin - s_TextHeight * 0.5f;

		m_Version_Label.SetPosition({margin, labelY});
		m_Version_Label.SetTextHeight(s_TextHeight);
		m_Version_Label.Render();

		m_Copyright_Label.SetPosition({(float) s_ScreenWidth - margin, labelY});
		m_Copyright_Label.SetTextHeight(s_TextHeight);
		m_Copyright_Label.Render();

		// ---- Splash text — anchored top-right of title sprite ----
		if (!m_Splashes.empty())
		{
			const std::string& splashText = m_Splashes[m_CurrentSplashIndex];
			float pulse = m_SplashTextPulse.GetValueSmoothPulse(glfwTime);
			float splashTextHeight = s_TextHeight * pulse;

			m_SplashText_Label.SetText(splashText);
			m_SplashText_Label.SetTextHeight(splashTextHeight);
		m_SplashText_Label.SetPosition(L(240.f, 56.f));
			m_SplashText_Label.Render();
		}
	}

	void MainMenuPanel::Initialize()
	{
		LoadSplashes();
		CycleSplashText();

		m_Title_Sprite.Initialize();

		m_Singleplayer_Button.Initialize();
		m_Multiplayer_Button.Initialize();
		m_DemoPanel_Button.Initialize();
		m_Options_Button.Initialize();
		m_QuitGame_Button.Initialize();
		m_SplashText_Label.Initialize();
		m_Copyright_Label.Initialize();
		m_Version_Label.Initialize();

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
		m_SplashText_Label.Delete();
		m_Copyright_Label.Delete();
		m_Version_Label.Delete();

		SetDeletedState(true);
	}

	void MainMenuPanel::ReloadTextures()
	{
		m_Title_Sprite.ReloadTextures();
		m_Singleplayer_Button.ReloadTextures();
		m_Multiplayer_Button.ReloadTextures();
		m_DemoPanel_Button.ReloadTextures();
		m_Options_Button.ReloadTextures();
		m_QuitGame_Button.ReloadTextures();
		m_SplashText_Label.ReloadTextures();
		m_Copyright_Label.ReloadTextures();
		m_Version_Label.ReloadTextures();
	}

	void MainMenuPanel::CycleSplashText()
	{
		if (m_Splashes.empty())
			return;

		// Select a random splash text from the list
		static std::random_device rd;  // Seed source
		static std::mt19937 gen(rd()); // Mersenne Twister engine
		std::uniform_int_distribution<int> dist(0, static_cast<int>(m_Splashes.size()) - 1);

		m_CurrentSplashIndex = dist(gen);
	}

	void MainMenuPanel::LoadSplashes()
	{
		m_Splashes.clear();

		std::string splashesContent = EngineContext::Get().Assets->GetFileText(s_SplashScreenTextPath);

		std::istringstream file(splashesContent);

		if (!file)
		{
			std::cerr << "Failed to open splashes file: " << s_SplashScreenTextPath << std::endl;
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
	}

	void MainMenuPanel::SubscribeToControlEvents()
	{
		m_EventHandles.push_back(m_Singleplayer_Button.EvtClick.Subscribe([this](const Button& sender)
																		  { Handle_Singleplayer_Click(sender); }));

		m_EventHandles.push_back(m_Multiplayer_Button.EvtClick.Subscribe([this](const Button& sender)
																		 { Handle_Multiplayer_Click(sender); }));

		m_EventHandles.push_back(
			m_DemoPanel_Button.EvtClick.Subscribe([this](const Button& sender) { Handle_DemoButton_Click(sender); }));

		m_EventHandles.push_back(
			m_Options_Button.EvtClick.Subscribe([this](const Button& sender) { Handle_Options_Click(sender); }));

		m_EventHandles.push_back(
			m_QuitGame_Button.EvtClick.Subscribe([this](const Button& sender) { Handle_QuitGame_Click(sender); }));
	}

	void MainMenuPanel::Handle_Singleplayer_Click(const Button& sender)
	{
		(void) sender; // Unused Parameter
		EvtRequestMenuNavigation.Trigger({this, eMenu::Singleplayer});
	}

	void MainMenuPanel::Handle_Multiplayer_Click(const Button& sender)
	{
		(void) sender; // Unused Parameter
		EvtRequestMenuNavigation.Trigger({this, eMenu::Multiplayer});
	}

	void MainMenuPanel::Handle_DemoButton_Click(const Button& sender)
	{
		(void) sender; // Unused Parameter
		EvtRequestMenuNavigation.Trigger({this, eMenu::DemoPanel});
	}

	void MainMenuPanel::Handle_Options_Click(const Button& sender)
	{
		(void) sender; // Unused Parameter
		EvtRequestMenuNavigation.Trigger({this, eMenu::Options});
	}

	void MainMenuPanel::Handle_QuitGame_Click(const Button& sender)
	{
		(void) sender; // Unused Parameter
		EvtRequestQuitGame.Trigger(this);
	}
}; // namespace onion::voxel
