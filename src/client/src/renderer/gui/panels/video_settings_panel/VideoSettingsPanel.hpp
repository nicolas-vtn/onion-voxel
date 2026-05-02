#pragma once

#include <renderer/gui/GuiElement.hpp>
#include <renderer/gui/controls/button/Button.hpp>
#include <renderer/gui/controls/label/Label.hpp>
#include <renderer/gui/controls/scroller/Scroller.hpp>
#include <renderer/gui/controls/slider/Slider.hpp>

namespace onion::voxel
{
	class VideoSettingsPanel : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		VideoSettingsPanel(const std::string& name);
		~VideoSettingsPanel() override;

		// ----- Public API -----
	  public:
		void Render() override;
		void Initialize() override;
		void Delete() override;
		void ReloadTextures() override;

		// ----- Public Events -----
	  public:
		Event<const GuiElement*> EvtRequestBackNavigation;
		Event<const UserSettingsChangedEventArgs&> EvtUserSettingsChanged;

		// ----- Controls -----
	  private:
		Label m_Title_Label;
		Scroller m_Scroller;

		Label m_DisplayTitle_Label;
		Slider m_MaxFps_Slider;
		Button m_Vsync_Button;

		Label m_QualAndPerfTitle_Label;
		Slider m_RenderDistance_Slider;
		Slider m_SimulationDistance_Slider;

		Label m_WailaTitle_Label;
		Button m_Waila_Button;

		Button m_Done_Button;

		// ----- Settings -----
	  private:
		static const uint32_t s_MaxFps_MinValue = 30;
		static const uint32_t s_MaxFps_MaxValue = 250;

		static const uint32_t s_RenderDistance_MinValue = 1;
		static const uint32_t s_RenderDistance_MaxValue = 32;

		static const uint32_t s_SimulationDistance_MinValue = 1;
		static const uint32_t s_SimulationDistance_MaxValue = 32;

		// ----- States -----
	  private:
		// ----- Internal Event Subscription and Handlers -----
	  private:
		std::vector<EventHandle> m_EventHandles;
		void SubscribeToControlEvents();

		void Handle_Done_Click(const Button& sender);

		void Handle_MaxFps_Changed(const Slider& sender);
		void Handle_Vsync_Click(const Button& sender);

		void Handle_RenderDistance_Changed(const Slider& sender);
		void Handle_SimulationDistance_Changed(const Slider& sender);
		void Handle_Waila_Click(const Button& sender);
	};
} // namespace onion::voxel
