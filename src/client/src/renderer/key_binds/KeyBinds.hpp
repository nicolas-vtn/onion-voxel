#pragma once

#include <mutex>
#include <unordered_map>

#include <renderer/inputs_manager/inputs_manager.hpp>

namespace onion::voxel
{
	enum class eAction
	{
		WalkForward,
		WalkBackward,
		StrafeLeft,
		StrafeRight,
		Jump,
		Sprint,
		Sneak,
		ToggleMouseCapture,
		ToggleFlyMode,
		Pause,
		Attack,
		PickBlock,
		Interact,
		DropItem,
		OpenInventory,
		HotbarSlot1,
		HotbarSlot2,
		HotbarSlot3,
		HotbarSlot4,
		HotbarSlot5,
		HotbarSlot6,
		HotbarSlot7,
		HotbarSlot8,
		HotbarSlot9,
		CloseMenu,
		ToggleDebugMenus
	};

	class KeyBinds
	{
		// ----- Constructor / Destructor -----
	  public:
		KeyBinds(InputsManager& inputsManager);

		// ----- Public API -----
	  public:
		void RemapAction(eAction action, Key key, InputConfig inputConfig);
		Key GetKeyForAction(eAction action) const;
		KeyState GetKeyState(eAction action) const;

		// ----- Private Members -----
	  private:
		InputsManager& m_InputsManager;

		mutable std::mutex m_Mutex;
		std::unordered_map<eAction, Key> m_ActionToKey;
		std::unordered_map<eAction, int> m_ActionToInputId;
	};

	static std::unordered_map<eAction, std::string> ActionToStringMap = {
		{eAction::WalkForward, "Walk Forward"},
		{eAction::WalkBackward, "Walk Backward"},
		{eAction::StrafeLeft, "Strafe Left"},
		{eAction::StrafeRight, "Strafe Right"},
		{eAction::Jump, "Jump"},
		{eAction::Sprint, "Sprint"},
		{eAction::Sneak, "Sneak"},
		{eAction::ToggleMouseCapture, "Toggle Mouse Capture"},
		{eAction::ToggleFlyMode, "Toggle Fly Mode"},
		{eAction::Pause, "Pause"},
		{eAction::Attack, "Attack/Destroy"},
		{eAction::PickBlock, "Pick Block"},
		{eAction::Interact, "Use Item/Place Block"},
		{eAction::DropItem, "Drop Item"},
		{eAction::OpenInventory, "Open Inventory"},
		{eAction::HotbarSlot1, "Hotbar Slot 1"},
		{eAction::HotbarSlot2, "Hotbar Slot 2"},
		{eAction::HotbarSlot3, "Hotbar Slot 3"},
		{eAction::HotbarSlot4, "Hotbar Slot 4"},
		{eAction::HotbarSlot5, "Hotbar Slot 5"},
		{eAction::HotbarSlot6, "Hotbar Slot 6"},
		{eAction::HotbarSlot7, "Hotbar Slot 7"},
		{eAction::HotbarSlot8, "Hotbar Slot 8"},
		{eAction::HotbarSlot9, "Hotbar Slot 9"},
		{eAction::CloseMenu, "Close Menu"},
		{eAction::ToggleDebugMenus, "Toggle Debug Menus"},
	};

	static std::unordered_map<std::string, eAction> StringToActionMap = []()
	{
		std::unordered_map<std::string, eAction> map;
		for (const auto& [key, name] : ActionToStringMap)
		{
			map[name] = key;
		}
		return map;
	}();

	std::string ActionToString(eAction action);

	eAction StringToAction(const std::string& str);

} // namespace onion::voxel
