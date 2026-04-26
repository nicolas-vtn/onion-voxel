#pragma once

#include <renderer/gui/GuiElement.hpp>
#include <renderer/gui/controls/button/Button.hpp>
#include <renderer/gui/controls/label/Label.hpp>
#include <renderer/gui/controls/scroller/Scroller.hpp>
#include <renderer/gui/controls/slider/Slider.hpp>

#include "KeyBindsTile.hpp"

namespace onion::voxel
{
	class KeyBindsPanel : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		KeyBindsPanel(const std::string& name);
		~KeyBindsPanel() override;

		// ----- Public API -----
	  public:
		void Render() override;
		void Initialize() override;
		void Delete() override;
		void ReloadTextures() override;

		void RefreshKeyBinds();

		// ----- Public Events -----
	  public:
		Event<const GuiElement*> EvtRequestBackNavigation;
		Event<const UserSettingsChangedEventArgs&> EvtUserSettingsChanged;

		// ----- Controls -----
	  private:
		Label m_Title_Label;
		Scroller m_Scroller;

		Label m_TitleMovement_Label;
		Label m_TitleGameplay_Label;
		Label m_TitleInventory_Label;
		Label m_TitleDebug_Label;
		std::unordered_map<eAction, std::unique_ptr<KeyBindsTile>> m_ActionToKeyBindTileMap;

		Button m_ResetAll_Button;
		Button m_Done_Button;

		// ----- Private Methods -----
	  private:
		void InitializeKeyBindTiles();
		bool AreAllKeyBindsDefault() const;
		bool IsAnyTileCapturingKey() const;

		// ----- Internal Event Subscription and Handlers -----
	  private:
		std::vector<EventHandle> m_EventHandles;
		void SubscribeToControlEvents();

		void Handle_KeyBindChanged(const KeyBindsTile& sender);

		void Handle_ResetAll_Click(const Button& sender);
		void Handle_Done_Click(const Button& sender);
	};
} // namespace onion::voxel
