#include "MouseSettingsPanel.hpp"

#include <format>

#include <renderer/gui/LayoutHelper.hpp>

namespace onion::voxel
{
	MouseSettingsPanel::MouseSettingsPanel(const std::string& name)
		: GuiElement(name), m_Title_Label(name + "_Title_Label"), m_Scroller(name + "_Scroller"),
		  m_Done_Button(name + "_Done_Button"), m_MouseSensitivity_Slider(name + "_MouseSensitivity_Slider"),
		  m_TouchscreenMode_Button(name + "_TouchscreenMode_Button"),
		  m_MouseScrollSensitivity_Slider(name + "_MouseScrollSensitivity_Slider"),
		  m_DiscreteScroll_Button(name + "_DiscreteScroll_Button"),
		  m_InvertMouseX_Button(name + "_InvertMouseX_Button"), m_InvertMouseY_Button(name + "_InvertMouseY_Button"),
		  m_AllowCursorChanges_Button(name + "_AllowCursorChanges_Button"), m_RawInput_Button(name + "_RawInput_Button")
	{
		SubscribeToControlEvents();

		m_Title_Label.SetText("Mouse Settings");
		m_Title_Label.SetTextAlignment(Font::eTextAlignment::Center);

		m_MouseSensitivity_Slider.SetMaxValue(200);

		m_TouchscreenMode_Button.SetText("Touchscreen Mode: OFF");
		m_TouchscreenMode_Button.SetEnabled(false);

		m_MouseScrollSensitivity_Slider.SetMaxValue(300);

		m_DiscreteScroll_Button.SetText("Discrete Scrolling: OFF");
		m_DiscreteScroll_Button.SetEnabled(false);

		m_InvertMouseX_Button.SetText("Invert Mouse X: OFF");
		m_InvertMouseX_Button.SetEnabled(false);

		m_InvertMouseY_Button.SetText("Invert Mouse Y: OFF");
		m_InvertMouseY_Button.SetEnabled(false);

		m_AllowCursorChanges_Button.SetText("Allow Cursor Changes: OFF");
		m_AllowCursorChanges_Button.SetEnabled(false);

		m_RawInput_Button.SetText("Raw Input: ON");
		m_RawInput_Button.SetEnabled(false);

		m_Done_Button.SetText("Done");
	}

	MouseSettingsPanel::~MouseSettingsPanel()
	{
		m_EventHandles.clear();
	}

	void MouseSettingsPanel::Render()
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

		// ----- Render Mouse Sensitivity Slider -----
		const glm::ivec2 mouseSensitivitySliderSize = tableLayout.GetCellSize();
		const glm::ivec2 mouseSensitivitySliderPosition = tableLayout.GetElementPosition(0, 0) + tableTopLeftCorner;
		const uint32_t mouseSensitivityPercent = static_cast<uint32_t>(
			round((userSettings.Controls.mouseSettings.Sensitivity / s_MouseSensitivityReferenceValue) * 100.f));
		const uint32_t sliderValue = m_MouseSensitivity_Slider.GetValue();
		std::string nbrStr = std::to_string(mouseSensitivityPercent) + "%";
		if (sliderValue == 0)
		{
			nbrStr = "*yawn*";
		}
		else if (sliderValue == m_MouseSensitivity_Slider.GetMaxValue())
		{
			nbrStr = "HYPERSPEED!!!";
		}
		std::string mouseSensitivitySliderText = "Sensitivity: " + nbrStr;
		m_MouseSensitivity_Slider.SetText(mouseSensitivitySliderText);
		m_MouseSensitivity_Slider.SetSize(mouseSensitivitySliderSize);
		m_MouseSensitivity_Slider.SetPosition(mouseSensitivitySliderPosition + scrollerOffset);
		m_MouseSensitivity_Slider.SetVisibility(
			m_Scroller.GetControlVisibleArea(m_MouseSensitivity_Slider.GetPosition(), mouseSensitivitySliderSize));
		m_MouseSensitivity_Slider.Render();

		// ----- Render Touchscreen Mode Button -----
		const glm::ivec2 touchscreenModeButtonSize = tableLayout.GetCellSize();
		const glm::ivec2 touchscreenModeButtonPosition = tableLayout.GetElementPosition(0, 1) + tableTopLeftCorner;
		m_TouchscreenMode_Button.SetSize(touchscreenModeButtonSize);
		m_TouchscreenMode_Button.SetPosition(touchscreenModeButtonPosition + scrollerOffset);
		m_TouchscreenMode_Button.SetVisibility(
			m_Scroller.GetControlVisibleArea(m_TouchscreenMode_Button.GetPosition(), touchscreenModeButtonSize));
		m_TouchscreenMode_Button.Render();

		// ----- Render Mouse Scroll Sensitivity Slider -----
		const glm::ivec2 mouseScrollSensitivitySliderSize = tableLayout.GetCellSize();
		const glm::ivec2 mouseScrollSensitivitySliderPosition =
			tableLayout.GetElementPosition(1, 0) + tableTopLeftCorner;
		const float mouseScrollSensitivitySliderValue = userSettings.Controls.mouseSettings.ScrollSensitivity;
		const std::string mouseScrollSensitivitySliderText =
			std::format("Scroll Sensitivity: {:.2f}", mouseScrollSensitivitySliderValue);
		m_MouseScrollSensitivity_Slider.SetText(mouseScrollSensitivitySliderText);
		m_MouseScrollSensitivity_Slider.SetSize(mouseScrollSensitivitySliderSize);
		m_MouseScrollSensitivity_Slider.SetPosition(mouseScrollSensitivitySliderPosition + scrollerOffset);
		m_MouseScrollSensitivity_Slider.SetVisibility(m_Scroller.GetControlVisibleArea(
			m_MouseScrollSensitivity_Slider.GetPosition(), mouseScrollSensitivitySliderSize));
		m_MouseScrollSensitivity_Slider.Render();

		// ----- Render Discrete Scroll Button -----
		const glm::ivec2 discreteScrollButtonSize = tableLayout.GetCellSize();
		const glm::ivec2 discreteScrollButtonPosition = tableLayout.GetElementPosition(1, 1) + tableTopLeftCorner;
		m_DiscreteScroll_Button.SetSize(discreteScrollButtonSize);
		m_DiscreteScroll_Button.SetPosition(discreteScrollButtonPosition + scrollerOffset);
		m_DiscreteScroll_Button.SetVisibility(
			m_Scroller.GetControlVisibleArea(m_DiscreteScroll_Button.GetPosition(), discreteScrollButtonSize));
		m_DiscreteScroll_Button.Render();

		/// ----- Render Invert Mouse X Button -----
		const glm::ivec2 invertMouseXButtonSize = tableLayout.GetCellSize();
		const glm::ivec2 invertMouseXButtonPosition = tableLayout.GetElementPosition(2, 0) + tableTopLeftCorner;
		m_InvertMouseX_Button.SetSize(invertMouseXButtonSize);
		m_InvertMouseX_Button.SetPosition(invertMouseXButtonPosition + scrollerOffset);
		m_InvertMouseX_Button.SetVisibility(
			m_Scroller.GetControlVisibleArea(m_InvertMouseX_Button.GetPosition(), invertMouseXButtonSize));
		m_InvertMouseX_Button.Render();

		// ----- Render Invert Mouse Y Button -----
		const glm::ivec2 invertMouseYButtonSize = tableLayout.GetCellSize();
		const glm::ivec2 invertMouseYButtonPosition = tableLayout.GetElementPosition(2, 1) + tableTopLeftCorner;
		m_InvertMouseY_Button.SetSize(invertMouseYButtonSize);
		m_InvertMouseY_Button.SetPosition(invertMouseYButtonPosition + scrollerOffset);
		m_InvertMouseY_Button.SetVisibility(
			m_Scroller.GetControlVisibleArea(m_InvertMouseY_Button.GetPosition(), invertMouseYButtonSize));
		m_InvertMouseY_Button.Render();

		// ----- Render Allow Cursor Changes Button -----
		const glm::ivec2 allowCursorChangesButtonSize = tableLayout.GetCellSize();
		const glm::ivec2 allowCursorChangesButtonPosition = tableLayout.GetElementPosition(3, 0) + tableTopLeftCorner;
		m_AllowCursorChanges_Button.SetSize(allowCursorChangesButtonSize);
		m_AllowCursorChanges_Button.SetPosition(allowCursorChangesButtonPosition + scrollerOffset);
		m_AllowCursorChanges_Button.SetVisibility(
			m_Scroller.GetControlVisibleArea(m_AllowCursorChanges_Button.GetPosition(), allowCursorChangesButtonSize));
		m_AllowCursorChanges_Button.Render();

		// ----- Render Raw Input Button -----
		const glm::ivec2 rawInputButtonSize = tableLayout.GetCellSize();
		const glm::ivec2 rawInputButtonPosition = tableLayout.GetElementPosition(3, 1) + tableTopLeftCorner;
		m_RawInput_Button.SetSize(rawInputButtonSize);
		m_RawInput_Button.SetPosition(rawInputButtonPosition + scrollerOffset);
		m_RawInput_Button.SetVisibility(
			m_Scroller.GetControlVisibleArea(m_RawInput_Button.GetPosition(), rawInputButtonSize));
		m_RawInput_Button.Render();

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

	void MouseSettingsPanel::Initialize()
	{
		m_Title_Label.Initialize();
		m_Scroller.Initialize();
		m_MouseSensitivity_Slider.Initialize();
		m_TouchscreenMode_Button.Initialize();
		m_MouseScrollSensitivity_Slider.Initialize();
		m_DiscreteScroll_Button.Initialize();
		m_InvertMouseX_Button.Initialize();
		m_InvertMouseY_Button.Initialize();
		m_AllowCursorChanges_Button.Initialize();
		m_RawInput_Button.Initialize();
		m_Done_Button.Initialize();

		UserSettings settings = EngineContext::Get().Settings();
		// Initialize Values
		const uint32_t mouseSensitivityPercent = static_cast<uint32_t>(
			round(settings.Controls.mouseSettings.Sensitivity / s_MouseSensitivityReferenceValue) * 100.f);
		m_MouseSensitivity_Slider.SetValue(mouseSensitivityPercent);

		const float mouseScrollSensitivity = settings.Controls.mouseSettings.ScrollSensitivity;
		const uint32_t mouseScrollSensitivityPercent = static_cast<uint32_t>(round(mouseScrollSensitivity * 100.f));
		m_MouseScrollSensitivity_Slider.SetValue(mouseScrollSensitivityPercent);

		SetInitState(true);
	}

	void MouseSettingsPanel::Delete()
	{
		m_Title_Label.Delete();
		m_Scroller.Delete();
		m_MouseSensitivity_Slider.Delete();
		m_TouchscreenMode_Button.Delete();
		m_MouseScrollSensitivity_Slider.Delete();
		m_DiscreteScroll_Button.Delete();
		m_InvertMouseX_Button.Delete();
		m_InvertMouseY_Button.Delete();
		m_AllowCursorChanges_Button.Delete();
		m_RawInput_Button.Delete();
		m_Done_Button.Delete();

		SetDeletedState(true);
	}

	void MouseSettingsPanel::ReloadTextures()
	{
		m_Title_Label.ReloadTextures();
		m_Scroller.ReloadTextures();
		m_MouseSensitivity_Slider.ReloadTextures();
		m_TouchscreenMode_Button.ReloadTextures();
		m_MouseScrollSensitivity_Slider.ReloadTextures();
		m_DiscreteScroll_Button.ReloadTextures();
		m_InvertMouseX_Button.ReloadTextures();
		m_InvertMouseY_Button.ReloadTextures();
		m_AllowCursorChanges_Button.ReloadTextures();
		m_RawInput_Button.ReloadTextures();
		m_Done_Button.ReloadTextures();
	}

	void MouseSettingsPanel::SubscribeToControlEvents()
	{
		m_EventHandles.push_back(
			m_Done_Button.OnClick.Subscribe([this](const Button& sender) { Handle_Done_Click(sender); }));

		m_EventHandles.push_back(m_MouseSensitivity_Slider.OnValueChanged.Subscribe(
			[this](const Slider& sender) { Handle_MouseSensitivity_Changed(sender); }));

		m_EventHandles.push_back(m_MouseScrollSensitivity_Slider.OnValueChanged.Subscribe(
			[this](const Slider& sender) { Handle_MouseScrollSensitivity_Changed(sender); }));
	}

	void MouseSettingsPanel::Handle_MouseSensitivity_Changed(const Slider& sender)
	{
		UserSettings userSettings = EngineContext::Get().Settings();

		uint32_t sliderValue = sender.GetValue();
		const float ratio = static_cast<float>(sliderValue) / 100.f;

		float newSensitivity = ratio * s_MouseSensitivityReferenceValue;

		UserSettingsChangedEventArgs args(userSettings);
		args.NewSettings.Controls.mouseSettings.Sensitivity = newSensitivity;
		args.MouseSensitivity_Changed = true;

		EvtUserSettingsChanged.Trigger(args);
	}

	void MouseSettingsPanel::Handle_MouseScrollSensitivity_Changed(const Slider& sender)
	{
		UserSettings userSettings = EngineContext::Get().Settings();

		uint32_t sliderValue = sender.GetValue();

		const float newScrollSensitivity = static_cast<float>(sliderValue) / 100.f;

		UserSettingsChangedEventArgs args(userSettings);
		args.NewSettings.Controls.mouseSettings.ScrollSensitivity = newScrollSensitivity;
		args.MouseScrollSensitivity_Changed = true;

		EvtUserSettingsChanged.Trigger(args);
	}

	void MouseSettingsPanel::Handle_Done_Click(const Button& sender)
	{
		(void) sender;
		EvtRequestBackNavigation.Trigger(this);
	}

} // namespace onion::voxel
