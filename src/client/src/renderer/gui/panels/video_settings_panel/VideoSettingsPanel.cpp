#include "VideoSettingsPanel.hpp"

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
		uint32_t maxFpsValue = sender.GetValue() + s_MaxFps_MinValue;

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
