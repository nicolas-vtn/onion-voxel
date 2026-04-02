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
		Interact,
		CloseMenu,
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
		{eAction::Attack, "Attack"},
		{eAction::Interact, "Interact"},
		{eAction::CloseMenu, "Close Menu"},
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
