#include "ControlsPanel.hpp"

#include <renderer/gui/LayoutHelper.hpp>

namespace onion::voxel
{
	ControlsPanel::ControlsPanel(const std::string& name)
		: GuiElement(name), m_Title_Label(name + "_Title_Label"), m_Scroller(name + "_Scroller"),
		  m_Done_Button(name + "_Done_Button"), m_MouseSettings_Button(name + "_MouseSettings_Button"),
		  m_KeyBinds_Button(name + "_KeyBinds_Button"), m_Sneak_Button(name + "_Sneak_Button"),
		  m_Sprint_Button(name + "_Sprint_Button"), m_AtkDestroy_Button(name + "_AtkDestroy_Button"),
		  m_Use_Button(name + "_Use_Button"), m_AutoJump_Button(name + "_AutoJump_Button"),
		  m_SprintWindow_Slider(name + "_SprintWindow_Slider"),
		  m_OperatorItemTabs_Button(name + "_OperatorItemTabs_Button")
	{
		SubscribeToControlEvents();

		m_Title_Label.SetText("Controls");
		m_Title_Label.SetTextAlignment(Font::eTextAlignment::Center);

		m_MouseSettings_Button.SetText("Mouse Settings...");

		m_KeyBinds_Button.SetText("Key Binds...");

		m_Sneak_Button.SetText("Sneak: Hold");
		m_Sneak_Button.SetEnabled(false);

		m_Sprint_Button.SetText("Sprint: Hold");
		m_Sprint_Button.SetEnabled(false);

		m_AtkDestroy_Button.SetText("Attack/Destroy: Hold");
		m_AtkDestroy_Button.SetEnabled(false);

		m_Use_Button.SetText("Use Item/Place Block: Hold");
		m_Use_Button.SetEnabled(false);

		m_AutoJump_Button.SetText("Auto-Jump: OFF");
		m_AutoJump_Button.SetEnabled(false);

		m_SprintWindow_Slider.SetText("Sprint Window: 7");
		m_SprintWindow_Slider.SetMaxValue(10);
		m_SprintWindow_Slider.SetValue(7);

		m_OperatorItemTabs_Button.SetText("Operator Items Tabs: OFF");
		m_OperatorItemTabs_Button.SetEnabled(false);

		m_Done_Button.SetText("Done");
	}

	ControlsPanel::~ControlsPanel()
	{
		m_EventHandles.clear();
	}

	void ControlsPanel::Render()
	{
		if (s_IsBackPressed)
		{
			Handle_Done_Click(m_Done_Button);
			return;
		}

		// ----- Constants -----
		UserSettings userSettings = EngineContext::Get().Settings();

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
		constexpr float menuYOffsetRatio = (87.f - 23.f) / 1009.f;
		glm::vec2 labelPosition = {s_ScreenWidth / 2, s_ScreenHeight * menuYOffsetRatio};
		m_Title_Label.SetPosition(labelPosition);
		m_Title_Label.SetTextHeight(s_TextHeight);
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

		// ----- Render Mouse Settings Button -----
		const glm::ivec2 mouseSettingsButtonSize = tableLayout.GetCellSize();
		const glm::ivec2 mouseSettingsButtonPosition = tableLayout.GetElementPosition(0, 0) + tableTopLeftCorner;
		m_MouseSettings_Button.SetSize(mouseSettingsButtonSize);
		m_MouseSettings_Button.SetPosition(mouseSettingsButtonPosition + scrollerOffset);
		m_MouseSettings_Button.SetVisibility(
			m_Scroller.GetControlVisibleArea(m_MouseSettings_Button.GetPosition(), mouseSettingsButtonSize));
		m_MouseSettings_Button.Render();

		// ----- Render Key Binds Button -----
		const glm::ivec2 keyBindsButtonSize = tableLayout.GetCellSize();
		const glm::ivec2 keyBindsButtonPosition = tableLayout.GetElementPosition(0, 1) + tableTopLeftCorner;
		m_KeyBinds_Button.SetSize(keyBindsButtonSize);
		m_KeyBinds_Button.SetPosition(keyBindsButtonPosition + scrollerOffset);
		m_KeyBinds_Button.SetVisibility(
			m_Scroller.GetControlVisibleArea(m_KeyBinds_Button.GetPosition(), keyBindsButtonSize));
		m_KeyBinds_Button.Render();

		// ----- Render Sneak Button -----
		const glm::ivec2 sneakButtonSize = tableLayout.GetCellSize();
		const glm::ivec2 sneakButtonPosition = tableLayout.GetElementPosition(1, 0) + tableTopLeftCorner;
		m_Sneak_Button.SetSize(sneakButtonSize);
		m_Sneak_Button.SetPosition(sneakButtonPosition + scrollerOffset);
		m_Sneak_Button.SetVisibility(m_Scroller.GetControlVisibleArea(m_Sneak_Button.GetPosition(), sneakButtonSize));
		m_Sneak_Button.Render();

		// ----- Render Sprint Button -----
		const glm::ivec2 sprintButtonSize = tableLayout.GetCellSize();
		const glm::ivec2 sprintButtonPosition = tableLayout.GetElementPosition(1, 1) + tableTopLeftCorner;
		m_Sprint_Button.SetSize(sprintButtonSize);
		m_Sprint_Button.SetPosition(sprintButtonPosition + scrollerOffset);
		m_Sprint_Button.SetVisibility(
			m_Scroller.GetControlVisibleArea(m_Sprint_Button.GetPosition(), sprintButtonSize));
		m_Sprint_Button.Render();

		// ----- Render Attack/Destroy Button -----
		const glm::ivec2 atkDestroyButtonSize = tableLayout.GetCellSize();
		const glm::ivec2 atkDestroyButtonPosition = tableLayout.GetElementPosition(2, 0) + tableTopLeftCorner;
		m_AtkDestroy_Button.SetSize(atkDestroyButtonSize);
		m_AtkDestroy_Button.SetPosition(atkDestroyButtonPosition + scrollerOffset);
		m_AtkDestroy_Button.SetVisibility(
			m_Scroller.GetControlVisibleArea(m_AtkDestroy_Button.GetPosition(), atkDestroyButtonSize));
		m_AtkDestroy_Button.Render();

		// ----- Render Use Item/Place Block Button -----
		const glm::ivec2 useButtonSize = tableLayout.GetCellSize();
		const glm::ivec2 useButtonPosition = tableLayout.GetElementPosition(2, 1) + tableTopLeftCorner;
		m_Use_Button.SetSize(useButtonSize);
		m_Use_Button.SetPosition(useButtonPosition + scrollerOffset);
		m_Use_Button.SetVisibility(m_Scroller.GetControlVisibleArea(m_Use_Button.GetPosition(), useButtonSize));
		m_Use_Button.Render();

		// ----- Render Auto-Jump Button -----
		const glm::ivec2 autoJumpButtonSize = tableLayout.GetCellSize();
		const glm::ivec2 autoJumpButtonPosition = tableLayout.GetElementPosition(3, 0) + tableTopLeftCorner;
		m_AutoJump_Button.SetSize(autoJumpButtonSize);
		m_AutoJump_Button.SetPosition(autoJumpButtonPosition + scrollerOffset);
		m_AutoJump_Button.SetVisibility(
			m_Scroller.GetControlVisibleArea(m_AutoJump_Button.GetPosition(), autoJumpButtonSize));
		m_AutoJump_Button.Render();

		// ----- Render Sprint Window Slider -----
		const glm::ivec2 sprintWindowSliderSize = tableLayout.GetCellSize();
		const glm::ivec2 sprintWindowSliderPosition = tableLayout.GetElementPosition(3, 1) + tableTopLeftCorner;
		const uint32_t sprintWindowSliderValue = m_SprintWindow_Slider.GetValue();
		const std::string sprintWindowSliderText = "Sprint Window: " + std::to_string(sprintWindowSliderValue);
		m_SprintWindow_Slider.SetSize(sprintWindowSliderSize);
		m_SprintWindow_Slider.SetText(sprintWindowSliderText);
		m_SprintWindow_Slider.SetPosition(sprintWindowSliderPosition + scrollerOffset);
		m_SprintWindow_Slider.SetVisibility(
			m_Scroller.GetControlVisibleArea(m_SprintWindow_Slider.GetPosition(), sprintWindowSliderSize));
		m_SprintWindow_Slider.Render();

		// ----- Render Operator Item Tabs Button ---
		const glm::ivec2 operatorItemTabsButtonSize = tableLayout.GetCellSize();
		const glm::ivec2 operatorItemTabsButtonPosition = tableLayout.GetElementPosition(4, 0) + tableTopLeftCorner;
		m_OperatorItemTabs_Button.SetSize(operatorItemTabsButtonSize);
		m_OperatorItemTabs_Button.SetPosition(operatorItemTabsButtonPosition + scrollerOffset);
		m_OperatorItemTabs_Button.SetVisibility(
			m_Scroller.GetControlVisibleArea(m_OperatorItemTabs_Button.GetPosition(), operatorItemTabsButtonSize));
		m_OperatorItemTabs_Button.Render();

		// Stop Scroller Cissoring
		m_Scroller.StopCissoring();

		// ----- Render Done Button -----
		float doneButtonYPosRatio = 948.f / 1009.f;
		float doneButtonWidth = (800.f / 1920.f) * s_ScreenWidth;
		glm::vec2 buttonPos = {s_ScreenWidth * 0.5f, s_ScreenHeight * doneButtonYPosRatio};
		m_Done_Button.SetPosition(buttonPos);
		m_Done_Button.SetSize({doneButtonWidth, s_ControlHeight});
		m_Done_Button.Render();
	}

	void ControlsPanel::Initialize()
	{
		m_Title_Label.Initialize();
		m_Scroller.Initialize();
		m_MouseSettings_Button.Initialize();
		m_KeyBinds_Button.Initialize();
		m_Sneak_Button.Initialize();
		m_Sprint_Button.Initialize();
		m_AtkDestroy_Button.Initialize();
		m_Use_Button.Initialize();
		m_AutoJump_Button.Initialize();
		m_SprintWindow_Slider.Initialize();
		m_OperatorItemTabs_Button.Initialize();
		m_Done_Button.Initialize();

		UserSettings settings = EngineContext::Get().Settings();
		// Initialize Values

		SetInitState(true);
	}

	void ControlsPanel::Delete()
	{
		m_Title_Label.Delete();
		m_Scroller.Delete();
		m_MouseSettings_Button.Delete();
		m_KeyBinds_Button.Delete();
		m_Sneak_Button.Delete();
		m_Sprint_Button.Delete();
		m_AtkDestroy_Button.Delete();
		m_Use_Button.Delete();
		m_AutoJump_Button.Delete();
		m_SprintWindow_Slider.Delete();
		m_OperatorItemTabs_Button.Delete();
		m_Done_Button.Delete();

		SetDeletedState(true);
	}

	void ControlsPanel::ReloadTextures()
	{
		m_Title_Label.ReloadTextures();
		m_Scroller.ReloadTextures();
		m_MouseSettings_Button.ReloadTextures();
		m_KeyBinds_Button.ReloadTextures();
		m_Sneak_Button.ReloadTextures();
		m_Sprint_Button.ReloadTextures();
		m_AtkDestroy_Button.ReloadTextures();
		m_Use_Button.ReloadTextures();
		m_AutoJump_Button.ReloadTextures();
		m_SprintWindow_Slider.ReloadTextures();
		m_OperatorItemTabs_Button.ReloadTextures();
		m_Done_Button.ReloadTextures();
	}

	void ControlsPanel::SubscribeToControlEvents()
	{
		m_EventHandles.push_back(m_MouseSettings_Button.EvtClick.Subscribe([this](const Button& sender)
																		  { Handle_MouseSettings_Click(sender); }));

		m_EventHandles.push_back(
			m_KeyBinds_Button.EvtClick.Subscribe([this](const Button& sender) { Handle_KeyBinds_Click(sender); }));

		m_EventHandles.push_back(
			m_Done_Button.EvtClick.Subscribe([this](const Button& sender) { Handle_Done_Click(sender); }));
	}

	void ControlsPanel::Handle_Done_Click(const Button& sender)
	{
		(void) sender;
		EvtRequestBackNavigation.Trigger(this);
	}

	void ControlsPanel::Handle_MouseSettings_Click(const Button& sender)
	{
		(void) sender;
		EvtRequestMenuNavigation.Trigger({this, eMenu::MouseSettings});
	}

	void ControlsPanel::Handle_KeyBinds_Click(const Button& sender)
	{
		(void) sender;
		EvtRequestMenuNavigation.Trigger({this, eMenu::KeyBinds});
	}

} // namespace onion::voxel
