#include "VideoSettingsPanel.hpp"

#include <renderer/gui/LayoutHelper.hpp>

namespace onion::voxel
{
	VideoSettingsPanel::VideoSettingsPanel(const std::string& name)
		: GuiElement(name), m_Title_Label(name + "_Title_Label"), m_Scroller(name + "_Scroller"),
		  m_DisplayTitle_Label(name + "_DisplayTitle_Label"), m_MaxFps_Slider(name + "_MaxFps_Slider"),
		  m_Vsync_Button(name + "_Vsync_Button"), m_QualAndPerfTitle_Label(name + "_QualAndPerfTitle_Label"),
		  m_RenderDistance_Slider(name + "_RenderDistance_Slider"),
		  m_SimulationDistance_Slider(name + "_SimulationDistance_Slider"), m_Done_Button(name + "_Done_Button")
	{
		SubscribeToControlEvents();

		m_Title_Label.SetText("Video Settings");
		m_Title_Label.SetTextAlignment(Font::eTextAlignment::Center);

		m_DisplayTitle_Label.SetText("Display");
		m_DisplayTitle_Label.SetTextAlignment(Font::eTextAlignment::Left);

		m_MaxFps_Slider.SetMaxValue((s_MaxFps_MaxValue - s_MaxFps_MinValue) / 10);

		m_QualAndPerfTitle_Label.SetText("Quality & Performance");
		m_QualAndPerfTitle_Label.SetTextAlignment(Font::eTextAlignment::Left);

		m_RenderDistance_Slider.SetMaxValue(s_RenderDistance_MaxValue - s_RenderDistance_MinValue);
		m_SimulationDistance_Slider.SetMaxValue(s_SimulationDistance_MaxValue - s_SimulationDistance_MinValue);

		m_Done_Button.SetText("Done");
	}

	VideoSettingsPanel::~VideoSettingsPanel()
	{
		m_EventHandles.clear();
	}

	void VideoSettingsPanel::Render()
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
		const float spacingYratioBetweenSections = 160.f / 1009.f;
		const int spacingYBetweenSections = static_cast<int>(round(s_ScreenHeight * spacingYratioBetweenSections));

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
		const int yOffsetScroller = m_Scroller.GetContentYOffset();
		const glm::ivec2 scrollerOffset(0, -yOffsetScroller);

		// Start Scroller Cissoring
		m_Scroller.StartCissoring();

		// ----- Render Display Settings Title Label -----
		const float displaySettingsTitleYRatio = (188.f - 23.f) / 1009.f;
		const glm::ivec2 displaySettingsTitlePosition(
			leftX, static_cast<int>(round(s_ScreenHeight * displaySettingsTitleYRatio)));
		m_DisplayTitle_Label.SetPosition(displaySettingsTitlePosition + scrollerOffset);
		m_DisplayTitle_Label.SetTextHeight(s_ScreenHeight * textHeightRatio);
		m_DisplayTitle_Label.Render();
		const int textHeight = m_DisplayTitle_Label.GetTextSize().y;

		// Build Layout Grid for Display Settings
		const int displaySettingsRows = 1;
		const int tableDisplayHeight =
			controlsHeight * displaySettingsRows + verticalSpacing * (displaySettingsRows - 1);
		const glm::ivec2 tableDisplaySize(tablesWidth, tableDisplayHeight);
		const TableLayout displaySettingsLayout = LayoutHelper::CreateTableLayout(
			displaySettingsRows, 2, tableDisplaySize, horizontalSpacing, verticalSpacing);
		const int topLeftYDisplay = displaySettingsTitlePosition.y + textHeight;
		const glm::ivec2 tableDisplayTopLeftCorner(leftX, topLeftYDisplay);

		// ----- Render Max FPS Slider -----
		const glm::ivec2 maxFpsSliderSize = displaySettingsLayout.GetCellSize();
		const glm::ivec2 maxFpsSliderPosition =
			displaySettingsLayout.GetElementPosition(0, 0) + tableDisplayTopLeftCorner;
		const uint32_t maxFpsSliderValue = userSettings.Video.MaxFPS;
		std::string maxFpsSliderText =
			"Max Framerate: " + (maxFpsSliderValue == 0 ? "Unlimited" : (std::to_string(maxFpsSliderValue) + " fps"));
		m_MaxFps_Slider.SetSize(maxFpsSliderSize);
		m_MaxFps_Slider.SetPosition(maxFpsSliderPosition + scrollerOffset);
		m_MaxFps_Slider.SetText(maxFpsSliderText);
		m_MaxFps_Slider.SetVisibility(
			m_Scroller.GetControlVisibleArea(m_MaxFps_Slider.GetPosition(), maxFpsSliderSize));
		m_MaxFps_Slider.Render();

		// ----- Render VSync Button -----
		const glm::ivec2 vsyncButtonSize = displaySettingsLayout.GetCellSize();
		const glm::ivec2 vsyncButtonPosition =
			displaySettingsLayout.GetElementPosition(0, 1) + tableDisplayTopLeftCorner;
		std::string vsyncButtonText = "VSync : " + std::string(userSettings.Video.VSyncEnabled ? "On" : "Off");
		m_Vsync_Button.SetSize(vsyncButtonSize);
		m_Vsync_Button.SetPosition(vsyncButtonPosition + scrollerOffset);
		m_Vsync_Button.SetText(vsyncButtonText);
		m_Vsync_Button.SetVisibility(m_Scroller.GetControlVisibleArea(m_Vsync_Button.GetPosition(), vsyncButtonSize));
		m_Vsync_Button.Render();

		// ----- Render Quality and Performance Settings Title Label -----
		const int qualAndPerfSettingsTitleY = vsyncButtonPosition.y + spacingYBetweenSections;
		const glm::ivec2 qualAndPerfSettingsTitlePosition(leftX, qualAndPerfSettingsTitleY);
		m_QualAndPerfTitle_Label.SetPosition(qualAndPerfSettingsTitlePosition + scrollerOffset);
		m_QualAndPerfTitle_Label.SetTextHeight(s_ScreenHeight * textHeightRatio);
		m_QualAndPerfTitle_Label.Render();

		// Build Layout Grid for Quality and Performance Settings
		const int displayQualRows = 1;
		const int tableQualHeight = controlsHeight * displayQualRows + verticalSpacing * (displayQualRows - 1);
		const glm::ivec2 tableQualSize(tablesWidth, tableQualHeight);
		const TableLayout qualSettingsLayout =
			LayoutHelper::CreateTableLayout(displayQualRows, 2, tableQualSize, horizontalSpacing, verticalSpacing);
		const int topLeftYQual = qualAndPerfSettingsTitlePosition.y + textHeight;
		const glm::ivec2 tableQualTopLeftCorner(leftX, topLeftYQual);

		// ----- Render Render Distance Slider -----
		const glm::ivec2 renderDistanceSliderSize = qualSettingsLayout.GetCellSize();
		const glm::ivec2 renderDistanceSliderPosition =
			qualSettingsLayout.GetElementPosition(0, 0) + tableQualTopLeftCorner;
		const uint8_t renderDistanceValue = userSettings.Video.RenderDistance;
		const std::string renderDistanceSliderText =
			"Render Distance: " + std::to_string(renderDistanceValue) + " Chunks";
		m_RenderDistance_Slider.SetSize(renderDistanceSliderSize);
		m_RenderDistance_Slider.SetPosition(renderDistanceSliderPosition + scrollerOffset);
		m_RenderDistance_Slider.SetText(renderDistanceSliderText);
		m_RenderDistance_Slider.SetVisibility(
			m_Scroller.GetControlVisibleArea(m_RenderDistance_Slider.GetPosition(), renderDistanceSliderSize));
		m_RenderDistance_Slider.Render();

		// ----- Render Simulation Distance Slider -----
		const glm::ivec2 simulationDistanceSliderSize = qualSettingsLayout.GetCellSize();
		const glm::ivec2 simulationDistanceSliderPosition =
			qualSettingsLayout.GetElementPosition(0, 1) + tableQualTopLeftCorner;
		const uint8_t simulationDistanceValue = userSettings.Video.SimulationDistance;
		const std::string simulationDistanceSliderText =
			"Simu Distance: " + std::to_string(simulationDistanceValue) + " Chunks";
		m_SimulationDistance_Slider.SetSize(simulationDistanceSliderSize);
		m_SimulationDistance_Slider.SetPosition(simulationDistanceSliderPosition + scrollerOffset);
		m_SimulationDistance_Slider.SetText(simulationDistanceSliderText);
		m_SimulationDistance_Slider.SetVisibility(
			m_Scroller.GetControlVisibleArea(m_SimulationDistance_Slider.GetPosition(), simulationDistanceSliderSize));
		m_SimulationDistance_Slider.Render();

		// Stop Scroller Cissoring
		m_Scroller.StopCissoring();

		// ----- Update Scrolling Area Height -----
		const int contentHeight =
			simulationDistanceSliderPosition.y + (simulationDistanceSliderSize.y / 2) - topLeftCorner.y;
		m_Scroller.SetScrollAreaHeight(contentHeight * 4);

		// ----- Render Done Button -----
		const float doneButtonYRatio = (972.f - 23.f) / 1009.f;
		const float doneButtonWidthRatio = 800.f / 1920.f;
		const glm::ivec2 doneButtonSize(static_cast<int>(round(s_ScreenWidth * doneButtonWidthRatio)), controlsHeight);
		const glm::ivec2 doneButtonPosition(centerX, static_cast<int>(round(s_ScreenHeight * doneButtonYRatio)));
		m_Done_Button.SetSize(doneButtonSize);
		m_Done_Button.SetPosition(doneButtonPosition);
		m_Done_Button.Render();
	}

	void VideoSettingsPanel::Initialize()
	{
		m_Title_Label.Initialize();
		m_Scroller.Initialize();
		m_DisplayTitle_Label.Initialize();
		m_MaxFps_Slider.Initialize();
		m_Vsync_Button.Initialize();
		m_QualAndPerfTitle_Label.Initialize();
		m_RenderDistance_Slider.Initialize();
		m_SimulationDistance_Slider.Initialize();
		m_Done_Button.Initialize();

		UserSettings settings = EngineContext::Get().Settings();
		// Initialize Slider Values
		m_MaxFps_Slider.SetValue((settings.Video.MaxFPS - s_MaxFps_MinValue) / 10);
		m_RenderDistance_Slider.SetValue(settings.Video.RenderDistance - s_RenderDistance_MinValue);
		m_SimulationDistance_Slider.SetValue(settings.Video.SimulationDistance - s_SimulationDistance_MinValue);

		SetInitState(true);
	}

	void VideoSettingsPanel::Delete()
	{
		m_Title_Label.Delete();
		m_Scroller.Delete();
		m_DisplayTitle_Label.Delete();
		m_MaxFps_Slider.Delete();
		m_Vsync_Button.Delete();
		m_QualAndPerfTitle_Label.Delete();
		m_RenderDistance_Slider.Delete();
		m_SimulationDistance_Slider.Delete();
		m_Done_Button.Delete();

		SetDeletedState(true);
	}

	void VideoSettingsPanel::ReloadTextures()
	{
		m_Title_Label.ReloadTextures();
		m_Scroller.ReloadTextures();
		m_DisplayTitle_Label.ReloadTextures();
		m_MaxFps_Slider.ReloadTextures();
		m_Vsync_Button.ReloadTextures();
		m_QualAndPerfTitle_Label.ReloadTextures();
		m_RenderDistance_Slider.ReloadTextures();
		m_SimulationDistance_Slider.ReloadTextures();
		m_Done_Button.ReloadTextures();
	}

	void VideoSettingsPanel::SubscribeToControlEvents()
	{
		m_EventHandles.push_back(
			m_Done_Button.OnClick.Subscribe([this](const Button& sender) { Handle_Done_Click(sender); }));

		m_EventHandles.push_back(
			m_MaxFps_Slider.OnValueChanged.Subscribe([this](const Slider& sender) { Handle_MaxFps_Changed(sender); }));

		m_EventHandles.push_back(
			m_Vsync_Button.OnClick.Subscribe([this](const Button& sender) { Handle_Vsync_Click(sender); }));

		m_EventHandles.push_back(m_RenderDistance_Slider.OnValueChanged.Subscribe(
			[this](const Slider& sender) { Handle_RenderDistance_Changed(sender); }));

		m_EventHandles.push_back(m_SimulationDistance_Slider.OnValueChanged.Subscribe(
			[this](const Slider& sender) { Handle_SimulationDistance_Changed(sender); }));
	}

	void VideoSettingsPanel::Handle_Done_Click(const Button& sender)
	{
		(void) sender;
		EvtRequestBackNavigation.Trigger(this);
	}

	void VideoSettingsPanel::Handle_MaxFps_Changed(const Slider& sender)
	{
		uint32_t maxFpsValue = (sender.GetValue() * 10) + s_MaxFps_MinValue;

		if (maxFpsValue == s_MaxFps_MaxValue)
		{
			maxFpsValue = 0; // 0 will represent uncapped FPS
		}

		UserSettings settings = EngineContext::Get().Settings();
		settings.Video.MaxFPS = maxFpsValue;

		UserSettingsChangedEventArgs args(settings);
		args.MaxFPS_Changed = true;

		EvtUserSettingsChanged.Trigger(args);
	}

	void VideoSettingsPanel::Handle_Vsync_Click(const Button& sender)
	{
		(void) sender;

		UserSettings settings = EngineContext::Get().Settings();

		// Toggle VSync
		settings.Video.VSyncEnabled = !settings.Video.VSyncEnabled;

		UserSettingsChangedEventArgs args(settings);
		args.VSyncEnabled_Changed = true;

		EvtUserSettingsChanged.Trigger(args);
	}

	void VideoSettingsPanel::Handle_RenderDistance_Changed(const Slider& sender)
	{
		uint32_t renderDistanceValue = sender.GetValue() + s_RenderDistance_MinValue;

		UserSettings settings = EngineContext::Get().Settings();

		settings.Video.RenderDistance = static_cast<uint8_t>(renderDistanceValue);

		UserSettingsChangedEventArgs args(settings);
		args.RenderDistance_Changed = true;

		EvtUserSettingsChanged.Trigger(args);
	}

	void VideoSettingsPanel::Handle_SimulationDistance_Changed(const Slider& sender)
	{

		uint32_t simulationDistanceValue = sender.GetValue() + s_SimulationDistance_MinValue;

		UserSettings settings = EngineContext::Get().Settings();
		settings.Video.SimulationDistance = static_cast<uint8_t>(simulationDistanceValue);

		UserSettingsChangedEventArgs args(settings);
		args.SimulationDistance_Changed = true;

		EvtUserSettingsChanged.Trigger(args);
	}

} // namespace onion::voxel
