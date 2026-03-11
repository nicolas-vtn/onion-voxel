#include "OptionsPanel.hpp"

#include "../../LayoutHelper.hpp"

namespace onion::voxel
{
	OptionsPanel::OptionsPanel(const std::string& name)
		: GuiElement(name), m_FOV_Button("FOV_Button"), m_Online_Button("Online_Button"),
		  m_SkinCustomization_Button("SkinCustomization_Button"), m_MusicAndSounds_Button("MusicAndSounds_Button"),
		  m_VideoSettings_Button("VideoSettings_Button"), m_Controls_Button("Controls_Button"),
		  m_Language_Button("Language_Button"), m_ChatSettings_Button("ChatSettings_Button"),
		  m_ResourcePacks_Button("ResourcePacks_Button"),
		  m_AccessibilitySettings_Button("AccessibilitySettings_Button"), m_Telemetry_Button("Telemetry_Button"),
		  m_Credits_Button("Credits_Button"), m_Done_Button("Done_Button")
	{
		SubscribeToControlEvents();

		m_FOV_Button.SetText("FOV...");
		m_FOV_Button.SetEnabled(false);
		m_Online_Button.SetText("Online...");
		m_Online_Button.SetEnabled(false);
		m_SkinCustomization_Button.SetText("Skin Customization...");
		m_SkinCustomization_Button.SetEnabled(false);
		m_MusicAndSounds_Button.SetText("Music & Sounds...");
		m_MusicAndSounds_Button.SetEnabled(false);
		m_VideoSettings_Button.SetText("Video Settings...");
		m_VideoSettings_Button.SetEnabled(false);
		m_Controls_Button.SetText("Controls...");
		m_Controls_Button.SetEnabled(false);
		m_Language_Button.SetText("Language...");
		m_Language_Button.SetEnabled(false);
		m_ChatSettings_Button.SetText("Chat Settings...");
		m_ChatSettings_Button.SetEnabled(false);
		m_ResourcePacks_Button.SetText("Resource Packs...");
		m_ResourcePacks_Button.SetEnabled(false);
		m_AccessibilitySettings_Button.SetText("Accessibility Settings...");
		m_AccessibilitySettings_Button.SetEnabled(false);
		m_Telemetry_Button.SetText("Telemetry Data...");
		m_Telemetry_Button.SetEnabled(false);
		m_Credits_Button.SetText("Credits & Attribution...");
		m_Credits_Button.SetEnabled(false);
		m_Done_Button.SetText("Done");
	}

	OptionsPanel::~OptionsPanel()
	{
		m_EventHandles.clear();
	}

	void OptionsPanel::Render()
	{
		if (s_IsBackPressed)
		{
			RequestBackNavigation.Trigger(this);
			return;
		}

		// ---- Constants for Layout ----
		glm::vec2 buttonSizeRatio{0.415f, 0.08f};
		glm::vec2 buttonSize{buttonSizeRatio.x * s_ScreenWidth, buttonSizeRatio.y * s_ScreenHeight};
		float buttonYSpacingRatio = 94.f / 1009.f;
		float firstButtonYPosRatio = 486.f / 1009.f;

		// ---- Menu Title ----
		const std::string titleText = "Options";
		float menuYOffsetRatio = (85.f - 23.f) / 1009.f;
		glm::vec2 textPosition = {s_ScreenWidth / 2, s_ScreenHeight * menuYOffsetRatio};
		float textHeight = s_ScreenHeight * (30.f / 1009.f);
		glm::vec3 textColor{1.f, 1.f, 1.f};
		float shadowOffset = textHeight / s_TextFont.GetGlyphSize().y;
		glm::vec3 shadowColor{0.246f, 0.246f, 0.246f};

		glm::vec2 shadowOffsetVec{shadowOffset, shadowOffset};

		s_TextFont.RenderText(
			titleText, Font::eTextAlignment::Center, textPosition + shadowOffsetVec, textHeight, shadowColor, 0.1f);
		s_TextFont.RenderText(titleText, Font::eTextAlignment::Center, textPosition, textHeight, textColor, 0.2f);

		// ---- Prepare Layout for first 2 buttons ----
		float tablesWidthRatio = 1229.f / 1920.f;
		float tablesWidth = s_ScreenWidth * tablesWidthRatio;
		float table1HeightRatio = 79.f / 1009.f;
		float table1Height = s_ScreenHeight * table1HeightRatio;

		float horizontalSpacingRatios = 33.f / 1920.f;
		float horizontalSpacings = s_ScreenWidth * horizontalSpacingRatios;
		float verticalSpacingRatios = 17.f / 1009.f;
		float verticalSpacings = s_ScreenHeight * verticalSpacingRatios;

		float table1TopRatio = 138.f / 1009.f;

		glm::ivec2 topLeftOfTable1{s_ScreenWidth * 0.5 - (tablesWidth / 2), s_ScreenHeight * table1TopRatio};

		TableLayout tableLayout1 = LayoutHelper::CreateTableLayout(
			1, 2, glm::ivec2(tablesWidth, table1Height), horizontalSpacings, verticalSpacings);

		const glm::ivec2 cellSize1 = tableLayout1.GetCellSize();

		// ---- Render FOV Button ----
		glm::ivec2 relativeButtonPos1 = tableLayout1.GetElementPosition(0, 0);
		m_FOV_Button.SetPosition(topLeftOfTable1 + relativeButtonPos1);
		m_FOV_Button.SetSize(cellSize1);
		m_FOV_Button.Render();

		// ---- Render Online Button ----
		relativeButtonPos1 = tableLayout1.GetElementPosition(0, 1);
		m_Online_Button.SetPosition(topLeftOfTable1 + relativeButtonPos1);
		m_Online_Button.SetSize(cellSize1);
		m_Online_Button.Render();

		// ---- Prepare Layout for next 10 buttons ----
		float table2TopRatio = 386.f / 1009.f;
		float table2HeightRatio = 462.f / 1009.f;
		float table2Height = s_ScreenHeight * table2HeightRatio;

		glm::ivec2 topLeftOfTable2{s_ScreenWidth * 0.5 - (tablesWidth / 2), s_ScreenHeight * table2TopRatio};
		TableLayout tableLayout2 = LayoutHelper::CreateTableLayout(
			5, 2, glm::ivec2(tablesWidth, table2Height), horizontalSpacings, verticalSpacings);
		glm::ivec2 cellSize2 = tableLayout2.GetCellSize();

		// ---- Render Skin Customization Button ----
		glm::ivec2 relativeButtonPos2 = tableLayout2.GetElementPosition(0, 0);
		m_SkinCustomization_Button.SetPosition(topLeftOfTable2 + relativeButtonPos2);
		m_SkinCustomization_Button.SetSize(cellSize2);
		m_SkinCustomization_Button.Render();

		// ---- Render Music and Sounds Button ----
		relativeButtonPos2 = tableLayout2.GetElementPosition(0, 1);
		m_MusicAndSounds_Button.SetPosition(topLeftOfTable2 + relativeButtonPos2);
		m_MusicAndSounds_Button.SetSize(cellSize2);
		m_MusicAndSounds_Button.Render();

		// ---- Render Video Settings Button ----
		relativeButtonPos2 = tableLayout2.GetElementPosition(1, 0);
		m_VideoSettings_Button.SetPosition(topLeftOfTable2 + relativeButtonPos2);
		m_VideoSettings_Button.SetSize(cellSize2);
		m_VideoSettings_Button.Render();

		// ---- Render Controls Button ----
		relativeButtonPos2 = tableLayout2.GetElementPosition(1, 1);
		m_Controls_Button.SetPosition(topLeftOfTable2 + relativeButtonPos2);
		m_Controls_Button.SetSize(cellSize2);
		m_Controls_Button.Render();

		// ---- Render Language Button ----
		relativeButtonPos2 = tableLayout2.GetElementPosition(2, 0);
		m_Language_Button.SetPosition(topLeftOfTable2 + relativeButtonPos2);
		m_Language_Button.SetSize(cellSize2);
		m_Language_Button.Render();

		// ---- Render Chat Settings Button ----
		relativeButtonPos2 = tableLayout2.GetElementPosition(2, 1);
		m_ChatSettings_Button.SetPosition(topLeftOfTable2 + relativeButtonPos2);
		m_ChatSettings_Button.SetSize(cellSize2);
		m_ChatSettings_Button.Render();

		// ---- Render Resource Packs Button ----
		relativeButtonPos2 = tableLayout2.GetElementPosition(3, 0);
		m_ResourcePacks_Button.SetPosition(topLeftOfTable2 + relativeButtonPos2);
		m_ResourcePacks_Button.SetSize(cellSize2);
		m_ResourcePacks_Button.Render();

		// ---- Render Accessibility Settings Button ----
		relativeButtonPos2 = tableLayout2.GetElementPosition(3, 1);
		m_AccessibilitySettings_Button.SetPosition(topLeftOfTable2 + relativeButtonPos2);
		m_AccessibilitySettings_Button.SetSize(cellSize2);
		m_AccessibilitySettings_Button.Render();

		// ---- Render Telemetry Button ----
		relativeButtonPos2 = tableLayout2.GetElementPosition(4, 0);
		m_Telemetry_Button.SetPosition(topLeftOfTable2 + relativeButtonPos2);
		m_Telemetry_Button.SetSize(cellSize2);
		m_Telemetry_Button.Render();

		// ---- Render Credits Button ----
		relativeButtonPos2 = tableLayout2.GetElementPosition(4, 1);
		m_Credits_Button.SetPosition(topLeftOfTable2 + relativeButtonPos2);
		m_Credits_Button.SetSize(cellSize2);
		m_Credits_Button.Render();

		// ---- Render Done Button ----
		float doneButtonYPosRatio = 948.f / 1009.f;
		glm::vec2 buttonPos = {s_ScreenWidth * 0.5f, s_ScreenHeight * doneButtonYPosRatio};
		m_Done_Button.SetPosition(buttonPos);
		m_Done_Button.SetSize(buttonSize);
		m_Done_Button.Render();
	}

	void OptionsPanel::Initialize()
	{
		m_FOV_Button.Initialize();
		m_Online_Button.Initialize();
		m_SkinCustomization_Button.Initialize();
		m_MusicAndSounds_Button.Initialize();
		m_VideoSettings_Button.Initialize();
		m_Controls_Button.Initialize();
		m_Language_Button.Initialize();
		m_ChatSettings_Button.Initialize();
		m_ResourcePacks_Button.Initialize();
		m_AccessibilitySettings_Button.Initialize();
		m_Telemetry_Button.Initialize();
		m_Credits_Button.Initialize();
		m_Done_Button.Initialize();

		SetInitState(true);
	}

	void OptionsPanel::Delete()
	{
		m_FOV_Button.Delete();
		m_Online_Button.Delete();
		m_SkinCustomization_Button.Delete();
		m_MusicAndSounds_Button.Delete();
		m_VideoSettings_Button.Delete();
		m_Controls_Button.Delete();
		m_Language_Button.Delete();
		m_ChatSettings_Button.Delete();
		m_ResourcePacks_Button.Delete();
		m_AccessibilitySettings_Button.Delete();
		m_Telemetry_Button.Delete();
		m_Credits_Button.Delete();
		m_Done_Button.Delete();

		SetDeletedState(true);
	}

	void OptionsPanel::SubscribeToControlEvents()
	{
		m_EventHandles.push_back(m_MusicAndSounds_Button.OnClick.Subscribe([this](const Button& sender)
																		   { Handle_MusicAndSounds_Click(sender); }));

		m_EventHandles.push_back(
			m_Controls_Button.OnClick.Subscribe([this](const Button& sender) { Handle_Controls_Click(sender); }));

		m_EventHandles.push_back(m_ResourcePacks_Button.OnClick.Subscribe([this](const Button& sender)
																		  { Handle_ResourcePacks_Click(sender); }));

		m_EventHandles.push_back(
			m_Done_Button.OnClick.Subscribe([this](const Button& sender) { Handle_Done_Click(sender); }));
	}

	void OptionsPanel::Handle_MusicAndSounds_Click(const Button& sender)
	{
		RequestMenuNavigation.Trigger({this, eMenu::MusicAndSounds});
	}

	void OptionsPanel::Handle_Controls_Click(const Button& sender)
	{
		RequestMenuNavigation.Trigger({this, eMenu::Controls});
	}

	void OptionsPanel::Handle_ResourcePacks_Click(const Button& sender)
	{
		RequestMenuNavigation.Trigger({this, eMenu::ResourcePacks});
	}

	void OptionsPanel::Handle_Done_Click(const Button& sender)
	{
		RequestBackNavigation.Trigger(this);
	}

} // namespace onion::voxel
