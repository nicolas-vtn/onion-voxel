#pragma once

#include <renderer/gui/GuiElement.hpp>
#include <renderer/gui/controls/button/Button.hpp>
#include <renderer/gui/controls/checkbox/Checkbox.hpp>
#include <renderer/gui/controls/label/Label.hpp>
#include <renderer/gui/controls/scroller/Scroller.hpp>
#include <renderer/gui/controls/slider/Slider.hpp>
#include <renderer/gui/controls/sprite/Sprite.hpp>
#include <renderer/gui/controls/text_field/TextField.hpp>

namespace onion::voxel
{
	class SingleplayerPanel : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		SingleplayerPanel(const std::string& name);
		~SingleplayerPanel() override;

		// ----- Public API -----
	  public:
		void Render() override;
		void Initialize() override;
		void Delete() override;
		void ReloadTextures() override;

		// ----- Public Events -----
	  public:
		Event<const GuiElement*> EvtRequestBackNavigation;

		// ----- Properties -----
	  private:
		// ----- Controls -----
	  private:
		Label m_LabelTitle;
		TextField m_TextFieldFilter;
		Scroller m_Scroller;

		Button m_ButtonBack;
		Button m_ButtonCreateNewWorld;
		Button m_PlaySelectedWorld;
		Button m_DeleteSelectedWorld;

		// ----- Internal Event Subscription and Handlers -----
	  private:
		void SubscribeToControlEvents();

		std::vector<EventHandle> m_EventHandles;

		void Handle_ButtonBackClick(const Button& button);
		void Handle_ButtonCreateNewWorldClick(const Button& button);
		void Handle_PlaySelectedWorldClick(const Button& button);
		void Handle_ButtonDeleteSelectedWorldClick(const Button& button);
	};
} // namespace onion::voxel
