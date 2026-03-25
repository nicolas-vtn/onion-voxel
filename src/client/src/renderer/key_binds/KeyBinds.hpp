#pragma once

#include <mutex>
#include <unordered_map>

#include <renderer/inputs_manager/inputs_manager.hpp>

namespace onion::voxel
{
	enum class eAction
	{
		MoveForward,
		MoveBackward,
		MoveLeft,
		MoveRight,
		Jump,
		Sprint,
		Crouch,
		ToggleMouseCapture,
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
} // namespace onion::voxel
