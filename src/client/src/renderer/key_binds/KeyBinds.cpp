#include "KeyBinds.hpp"

namespace onion::voxel
{
	KeyBinds::KeyBinds(InputsManager& inputsManager) : m_InputsManager(inputsManager) {}

	void KeyBinds::RemapAction(eAction action, Key key, InputConfig inputConfig)
	{
		std::lock_guard lock(m_Mutex);

		// If key exists, unregister the old input
		auto itInputId = m_ActionToInputId.find(action);
		if (itInputId != m_ActionToInputId.end())
		{
			int oldInputId = itInputId->second;
			m_InputsManager.UnregisterInput(oldInputId);
			m_ActionToInputId.erase(itInputId);
		}

		// Register the new key and store the input ID
		int inputId = m_InputsManager.RegisterInput(key, inputConfig);
		m_ActionToInputId[action] = inputId;

		// Store the key for this action
		m_ActionToKey[action] = key;
	}

	Key KeyBinds::GetKeyForAction(eAction action) const
	{
		std::lock_guard lock(m_Mutex);

		auto it = m_ActionToKey.find(action);
		if (it == m_ActionToKey.end())
			return Key::Unknown;

		return it->second;
	}

	KeyState KeyBinds::GetKeyState(eAction action) const
	{
		std::lock_guard lock(m_Mutex);

		auto itInputId = m_ActionToInputId.find(action);
		if (itInputId == m_ActionToInputId.end())
			return KeyState();

		int inputId = itInputId->second;
		auto snapshot = m_InputsManager.GetInputsSnapshot();
		return snapshot->GetKeyState(inputId);
	}
} // namespace onion::voxel
