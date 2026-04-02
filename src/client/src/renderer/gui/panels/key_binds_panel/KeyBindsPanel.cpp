#include "KeyBindsPanel.hpp"

#include <renderer/gui/LayoutHelper.hpp>

namespace onion::voxel
{
	KeyBindsPanel::KeyBindsPanel(const std::string& name)
		: GuiElement(name), m_Title_Label(name + "_Title_Label"), m_Scroller(name + "_Scroller"),
		  m_Done_Button(name + "_Done_Button")
	{
		SubscribeToControlEvents();

		m_Title_Label.SetText("Key Binds");
		m_Title_Label.SetTextAlignment(Font::eTextAlignment::Center);

		m_Done_Button.SetText("Done");
	}

	KeyBindsPanel::~KeyBindsPanel()
	{
		m_EventHandles.clear();
	}

	void KeyBindsPanel::Render()
	{
		if (s_IsBackPressed)
		{
			Handle_Done_Click(m_Done_Button);
			return;
		}

		// ----- Constants -----
		UserSettings userSettings = EngineContext::Get().Settings();

		const int centerX = static_cast<int>(round(s_ScreenWidth / 2.0));
		const float leftXratio = 340.f / 1920.f;
		const int leftX = static_cast<int>(round(s_ScreenWidth * leftXratio));
		const float controlsHeightRatio = 80.f / 1009.f;
		const int controlsHeight = static_cast<int>(round(s_ScreenHeight * controlsHeightRatio));
		const float horizontalSpacingRatio = 40.f / 1920.f;
		const int horizontalSpacing = static_cast<int>(round(s_ScreenWidth * horizontalSpacingRatio));
		const float verticalSpacingRatio = 20.f / 1009.f;
		const int verticalSpacing = static_cast<int>(round(s_ScreenHeight * verticalSpacingRatio));
		const float tablesWidthRatio = 1240.f / 1920.f;
		const int tablesWidth = static_cast<int>(round(s_ScreenWidth * tablesWidthRatio));

		// ----- Render Title Label -----
		const float titleYratio = (88.f - 23.f) / 1009.f;
		const glm::ivec2 titlePosition(centerX, static_cast<int>(round(s_ScreenHeight * titleYratio)));
		float textHeightRatio = 32.f / 1009.f;
		m_Title_Label.SetPosition(titlePosition);
		m_Title_Label.SetTextHeight(s_ScreenHeight * textHeightRatio);
		m_Title_Label.Render();

		// ----- Render Scroller -----
		float scrollerTopYRatio = (151.f - 23.f) / 1009.f;
		float scrollerBottomYRatio = (902.f - 23.f) / 1009.f;
		glm::ivec2 topLeftCorner(0, static_cast<int>(round(s_ScreenHeight * scrollerTopYRatio)));
		glm::ivec2 bottomRightCorner(s_ScreenWidth, static_cast<int>(round(s_ScreenHeight * scrollerBottomYRatio)));
		m_Scroller.SetTopLeftCorner(topLeftCorner);
		m_Scroller.SetBottomRightCorner(bottomRightCorner);
		m_Scroller.Render();

		// Start Scroller Cissoring
		m_Scroller.StartCissoring();

		// ----- Build Layout Grid for Controls -----
		const int rows = 5;
		const int tableHeight = controlsHeight * rows + verticalSpacing * (rows - 1);
		const glm::ivec2 tableSize(tablesWidth, tableHeight);
		const TableLayout tableLayout =
			LayoutHelper::CreateTableLayout(rows, 2, tableSize, horizontalSpacing, verticalSpacing);
		const float topLeftYratio = (171 - 23.f) / 1009.f;
		const int topLeftY = static_cast<int>(round(s_ScreenHeight * topLeftYratio));
		const glm::ivec2 tableTopLeftCorner(leftX, topLeftY);

		// ----- Update Scrolling Area Height -----
		m_Scroller.SetScrollAreaHeight(tableHeight);
		const int yOffsetScroller = m_Scroller.GetContentYOffset();
		const glm::ivec2 scrollerOffset(0, -yOffsetScroller);

		//// ----- Render Mouse Settings Button -----
		//const glm::ivec2 mouseSettingsButtonSize = tableLayout.GetCellSize();
		//const glm::ivec2 mouseSettingsButtonPosition = tableLayout.GetElementPosition(0, 0) + tableTopLeftCorner;
		//m_MouseSettings_Button.SetSize(mouseSettingsButtonSize);
		//m_MouseSettings_Button.SetPosition(mouseSettingsButtonPosition + scrollerOffset);
		//m_MouseSettings_Button.SetVisibility(
		//	m_Scroller.GetControlVisibleArea(m_MouseSettings_Button.GetPosition(), mouseSettingsButtonSize));
		//m_MouseSettings_Button.Render();

		// Stop Scroller Cissoring
		m_Scroller.StopCissoring();

		// ----- Render Done Button -----
		const float doneButtonYRatio = (972.f - 23.f) / 1009.f;
		const float doneButtonWidthRatio = 800.f / 1920.f;
		const glm::ivec2 doneButtonSize(static_cast<int>(round(s_ScreenWidth * doneButtonWidthRatio)), controlsHeight);
		const glm::ivec2 doneButtonPosition(centerX, static_cast<int>(round(s_ScreenHeight * doneButtonYRatio)));
		m_Done_Button.SetSize(doneButtonSize);
		m_Done_Button.SetPosition(doneButtonPosition);
		m_Done_Button.Render();
	}

	void KeyBindsPanel::Initialize()
	{
		m_Title_Label.Initialize();
		m_Scroller.Initialize();
		m_Done_Button.Initialize();

		UserSettings settings = EngineContext::Get().Settings();
		// Initialize Values

		SetInitState(true);
	}

	void KeyBindsPanel::Delete()
	{
		m_Title_Label.Delete();
		m_Scroller.Delete();
		m_Done_Button.Delete();

		SetDeletedState(true);
	}

	void KeyBindsPanel::ReloadTextures()
	{
		m_Title_Label.ReloadTextures();
		m_Scroller.ReloadTextures();
		m_Done_Button.ReloadTextures();
	}

	void KeyBindsPanel::SubscribeToControlEvents()
	{
		m_EventHandles.push_back(
			m_Done_Button.OnClick.Subscribe([this](const Button& sender) { Handle_Done_Click(sender); }));
	}

	void KeyBindsPanel::Handle_Done_Click(const Button& sender)
	{
		(void) sender;
		EvtRequestBackNavigation.Trigger(this);
	}

} // namespace onion::voxel
