#include "KeyBindsPanel.hpp"

#include <renderer/gui/LayoutHelper.hpp>

namespace onion::voxel
{
	KeyBindsPanel::KeyBindsPanel(const std::string& name)
		: GuiElement(name), m_Title_Label(name + "_Title_Label"), m_Scroller(name + "_Scroller"),
		  m_Done_Button(name + "_Done_Button"), m_ResetAll_Button(name + "_ResetAll_Button"),
		  m_TitleMovement_Label(name + "_TitleMovement_Label"), m_TitleGameplay_Label(name + "_TitleGameplay_Label"),
		  m_TitleDebug_Label(name + "_TitleDebug_Label")
	{
		SubscribeToControlEvents();

		m_Title_Label.SetText("Key Binds");
		m_Title_Label.SetTextAlignment(Font::eTextAlignment::Center);

		m_TitleMovement_Label.SetText("Movement");
		m_TitleMovement_Label.SetTextAlignment(Font::eTextAlignment::Center);

		m_TitleGameplay_Label.SetText("Gameplay");
		m_TitleGameplay_Label.SetTextAlignment(Font::eTextAlignment::Center);

		m_TitleDebug_Label.SetText("Debug");
		m_TitleDebug_Label.SetTextAlignment(Font::eTextAlignment::Center);

		m_ResetAll_Button.SetText("Reset Keys");
		m_Done_Button.SetText("Done");
	}

	KeyBindsPanel::~KeyBindsPanel()
	{
		m_EventHandles.clear();
	}

	void KeyBindsPanel::Render()
	{
		if (s_IsBackPressed && !IsAnyTileCapturingKey())
		{
			Handle_Done_Click(m_Done_Button);
			return;
		}

		// ----- Constants -----
		UserSettings userSettings = EngineContext::Get().Settings();

		const int centerX = static_cast<int>(round(s_ScreenWidth / 2.0));
		const float controlsHeightRatio = 80.f / 1009.f;
		const int controlsHeight = static_cast<int>(round(s_ScreenHeight * controlsHeightRatio));

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

		// ----- Constants for scroller area -----
		const float firstElementYOffsetRatio = 52.f / 1009.f;
		const int firstElementYOffset = static_cast<int>(round(s_ScreenHeight * firstElementYOffsetRatio));
		const float otherElementsYOffsetRatio = 80.f / 1009.f;
		const int otherElementsYOffset = static_cast<int>(round(s_ScreenHeight * otherElementsYOffsetRatio));

		// ----- Tile Sizes -----
		const float tileHeightRatio = 80.f / 1009.f;
		const int tileHeight = static_cast<int>(round(s_ScreenHeight * tileHeightRatio));
		const float tileWidthRatio = 1345.f / 1920.f;
		const int tileWidth = static_cast<int>(round(s_ScreenWidth * tileWidthRatio));
		const glm::ivec2 tileSize(tileWidth, tileHeight);

		int currentYPosition = topLeftCorner.y + firstElementYOffset;
		const glm::ivec2 scrollerOffset(0, -static_cast<int>(m_Scroller.GetContentYOffset()));

		// ----- Render Title Movement
		const glm::ivec2 titleMovementPosition(centerX, currentYPosition);
		m_TitleMovement_Label.SetPosition(titleMovementPosition + scrollerOffset);
		m_TitleMovement_Label.SetTextHeight(s_TextHeight);
		m_TitleMovement_Label.Render();
		currentYPosition += otherElementsYOffset;

		// ----- Render Movement Tiles -----
		std::vector<eAction> movementActions = {eAction::Jump,
												eAction::Sneak,
												eAction::Sprint,
												eAction::StrafeLeft,
												eAction::StrafeRight,
												eAction::WalkBackward,
												eAction::WalkForward};

		for (eAction action : movementActions)
		{
			auto it = m_ActionToKeyBindTileMap.find(action);
			if (it != m_ActionToKeyBindTileMap.end())
			{
				KeyBindsTile* tilePtr = it->second.get();
				const glm::ivec2 tilePosition(centerX, currentYPosition);
				tilePtr->SetPosition(tilePosition + scrollerOffset);
				tilePtr->SetSize(tileSize);
				tilePtr->SetVisibility(m_Scroller.GetControlVisibleArea(tilePtr->GetPosition(), tileSize));
				tilePtr->Render();
				currentYPosition += tileSize.y;
			}
		}

		// ----- Render Title Gameplay -----
		const glm::ivec2 titleGameplayPosition(centerX, currentYPosition);
		m_TitleGameplay_Label.SetPosition(titleGameplayPosition + scrollerOffset);
		m_TitleGameplay_Label.SetTextHeight(s_TextHeight);
		m_TitleGameplay_Label.Render();
		currentYPosition += otherElementsYOffset;

		// ----- Render Gameplay Tiles -----
		std::vector<eAction> gameplayActions = {eAction::Attack, eAction::Interact};

		for (eAction action : gameplayActions)
		{
			auto it = m_ActionToKeyBindTileMap.find(action);
			if (it != m_ActionToKeyBindTileMap.end())
			{
				KeyBindsTile* tilePtr = it->second.get();
				const glm::ivec2 tilePosition(centerX, currentYPosition);
				tilePtr->SetPosition(tilePosition + scrollerOffset);
				tilePtr->SetSize(tileSize);
				tilePtr->SetVisibility(m_Scroller.GetControlVisibleArea(tilePtr->GetPosition(), tileSize));
				tilePtr->Render();
				currentYPosition += tileSize.y;
			}
		}

		// ----- Render Title Debug -----
		const glm::ivec2 titleDebugPosition(centerX, currentYPosition);
		m_TitleDebug_Label.SetPosition(titleDebugPosition + scrollerOffset);
		m_TitleDebug_Label.SetTextHeight(s_TextHeight);
		m_TitleDebug_Label.Render();
		currentYPosition += otherElementsYOffset;

		// ----- Render Debug Tiles -----
		std::vector<eAction> debugActions = {eAction::ToggleDebugMenus, eAction::ToggleMouseCapture};

		for (eAction action : debugActions)
		{
			auto it = m_ActionToKeyBindTileMap.find(action);
			if (it != m_ActionToKeyBindTileMap.end())
			{
				KeyBindsTile* tilePtr = it->second.get();
				const glm::ivec2 tilePosition(centerX, currentYPosition);
				tilePtr->SetPosition(tilePosition + scrollerOffset);
				tilePtr->SetSize(tileSize);
				tilePtr->SetVisibility(m_Scroller.GetControlVisibleArea(tilePtr->GetPosition(), tileSize));
				tilePtr->Render();
				currentYPosition += tileSize.y;
			}
		}

		// ----- Update Scrolling Area Height -----
		const int tableHeight = currentYPosition - topLeftCorner.y;
		m_Scroller.SetScrollAreaHeight(tableHeight);

		// Stop Scroller Cissoring
		m_Scroller.StopCissoring();

		// ----- Create Layout for Reset All and Done Buttons -----
		const float tableWidthRatio = 1232.f / 1920.f;
		const int tableWidth = static_cast<int>(round(s_ScreenWidth * tableWidthRatio));
		const glm::ivec2 resetAndDoneTableSize(tableWidth, controlsHeight);
		const float resetAndDoneHorizontalSpacingRatio = 32.f / 1920.f;
		const int resetAndDoneHorizontalSpacing =
			static_cast<int>(round(s_ScreenWidth * resetAndDoneHorizontalSpacingRatio));
		const TableLayout resetAndDoneLayout =
			LayoutHelper::CreateTableLayout(1, 2, resetAndDoneTableSize, resetAndDoneHorizontalSpacing, 0);
		float doneButtonYPosRatio = 948.f / 1009.f;
		int resetAndDoneTopY = static_cast<int>(round(s_ScreenHeight * doneButtonYPosRatio - (s_ControlHeight / 2.f)));
		float resetAndDoneLeftXRatio = 344.f / 1920.f;
		int resetAndDoneLeftX = static_cast<int>(round(s_ScreenWidth * resetAndDoneLeftXRatio));
		const glm::ivec2 resetAndDoneTableTopLeftCorner(resetAndDoneLeftX, resetAndDoneTopY);

		// ----- Render Reset All Button -----
		const glm::ivec2 resetAllButtonPosition =
			resetAndDoneLayout.GetElementPosition(0, 0) + resetAndDoneTableTopLeftCorner;
		m_ResetAll_Button.SetSize(resetAndDoneLayout.GetCellSize());
		m_ResetAll_Button.SetPosition(resetAllButtonPosition);
		m_ResetAll_Button.SetEnabled(!AreAllKeyBindsDefault());
		m_ResetAll_Button.Render();

		// ----- Render Done Button -----
		const glm::ivec2 doneButtonPosition =
			resetAndDoneLayout.GetElementPosition(0, 1) + resetAndDoneTableTopLeftCorner;
		m_Done_Button.SetSize(resetAndDoneLayout.GetCellSize());
		m_Done_Button.SetPosition(doneButtonPosition);
		m_Done_Button.Render();
	}

	void KeyBindsPanel::Initialize()
	{
		m_Title_Label.Initialize();
		m_Scroller.Initialize();
		m_Done_Button.Initialize();
		m_ResetAll_Button.Initialize();
		m_TitleMovement_Label.Initialize();
		m_TitleGameplay_Label.Initialize();
		m_TitleDebug_Label.Initialize();

		InitializeKeyBindTiles();

		SetInitState(true);
	}

	void KeyBindsPanel::Delete()
	{
		m_Title_Label.Delete();
		m_Scroller.Delete();
		m_Done_Button.Delete();
		m_ResetAll_Button.Delete();
		m_TitleMovement_Label.Delete();
		m_TitleGameplay_Label.Delete();
		m_TitleDebug_Label.Delete();

		for (auto& [action, tilePtr] : m_ActionToKeyBindTileMap)
		{
			tilePtr->Delete();
		}

		SetDeletedState(true);
	}

	void KeyBindsPanel::ReloadTextures()
	{
		m_Title_Label.ReloadTextures();
		m_Scroller.ReloadTextures();
		m_Done_Button.ReloadTextures();
		m_ResetAll_Button.ReloadTextures();
		m_TitleMovement_Label.ReloadTextures();
		m_TitleGameplay_Label.ReloadTextures();
		m_TitleDebug_Label.ReloadTextures();

		for (auto& [action, tilePtr] : m_ActionToKeyBindTileMap)
		{
			tilePtr->ReloadTextures();
		}
	}

	void KeyBindsPanel::RefreshKeyBinds()
	{
		UserSettings userSettings = EngineContext::Get().Settings();
		std::unordered_map<eAction, Key>& keyBinds = userSettings.Controls.keyBindsSettings.ActionToKey;

		for (auto& [action, key] : keyBinds)
		{
			if (m_ActionToKeyBindTileMap.contains(action))
			{
				m_ActionToKeyBindTileMap.at(action)->SetKey(key);
			}
		}
	}

	void KeyBindsPanel::InitializeKeyBindTiles()
	{
		UserSettings userSettings = EngineContext::Get().Settings();
		std::unordered_map<eAction, Key>& keyBinds = userSettings.Controls.keyBindsSettings.ActionToKey;

		for (auto& [action, key] : keyBinds)
		{
			if (m_ActionToKeyBindTileMap.contains(action))
			{
				m_ActionToKeyBindTileMap.at(action)->SetKey(key);
			}
			else
			{
				std::string tileName = "KeyBindTile_" + ActionToString(action);
				std::unique_ptr<KeyBindsTile> tilePtr = std::make_unique<KeyBindsTile>(tileName, action, key);
				tilePtr->Initialize();
				m_EventHandles.push_back(tilePtr->EvtKeyBindChanged.Subscribe([this](const KeyBindsTile& sender)
																			  { Handle_KeyBindChanged(sender); }));
				m_ActionToKeyBindTileMap[action] = std::move(tilePtr);
			}
		}
	}

	bool KeyBindsPanel::AreAllKeyBindsDefault() const
	{
		bool allDefault = true;

		for (auto& [action, tilePtr] : m_ActionToKeyBindTileMap)
		{
			if (!tilePtr->IsDefaultKey())
			{
				allDefault = false;
				break;
			}
		}

		return allDefault;
	}

	bool KeyBindsPanel::IsAnyTileCapturingKey() const
	{
		bool anyCapturing = false;
		for (auto& [action, tilePtr] : m_ActionToKeyBindTileMap)
		{
			if (tilePtr->IsCapturingKey())
			{
				anyCapturing = true;
				break;
			}
		}
		return anyCapturing;
	}

	void KeyBindsPanel::SubscribeToControlEvents()
	{
		m_EventHandles.push_back(
			m_Done_Button.EvtClick.Subscribe([this](const Button& sender) { Handle_Done_Click(sender); }));

		m_EventHandles.push_back(
			m_ResetAll_Button.EvtClick.Subscribe([this](const Button& sender) { Handle_ResetAll_Click(sender); }));
	}

	void KeyBindsPanel::Handle_KeyBindChanged(const KeyBindsTile& sender)
	{
		UserSettings userSettings = EngineContext::Get().Settings();

		// Update the changed key bind in the settings
		eAction action = sender.GetAction();
		Key newKey = sender.GetKey();

		// Updates the settings with the new key bind
		userSettings.Controls.keyBindsSettings.ActionToKey[action] = newKey;

		UserSettingsChangedEventArgs evtArgs = UserSettingsChangedEventArgs(userSettings);
		evtArgs.KeyBinds_Changed = true;
		evtArgs.ChangedKeyBinds[action] = newKey;
		EvtUserSettingsChanged.Trigger(evtArgs);
	}

	void KeyBindsPanel::Handle_ResetAll_Click(const Button& sender)
	{
		(void) sender;

		if (AreAllKeyBindsDefault())
			return;

		UserSettings userSettings = EngineContext::Get().Settings();
		UserSettingsChangedEventArgs evtArgs = UserSettingsChangedEventArgs(userSettings);
		auto& actionToKeyBindMap = evtArgs.NewSettings.Controls.keyBindsSettings.ActionToKey;

		for (auto& [action, tilePtr] : m_ActionToKeyBindTileMap)
		{
			Key defaultKey = GetDefaultKeyForAction(action);
			tilePtr->SetKey(defaultKey);

			actionToKeyBindMap[action] = defaultKey;

			evtArgs.KeyBinds_Changed = true;
			evtArgs.ChangedKeyBinds[action] = defaultKey;
		}

		EvtUserSettingsChanged.Trigger(evtArgs);
	}

	void KeyBindsPanel::Handle_Done_Click(const Button& sender)
	{
		(void) sender;
		EvtRequestBackNavigation.Trigger(this);
	}

} // namespace onion::voxel
