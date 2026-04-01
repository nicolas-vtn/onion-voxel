#pragma once

#include <nlohmann/json.hpp>
#include <unordered_map>

#include <renderer/key_binds/KeyBinds.hpp>

namespace onion::voxel
{
	struct KeyBindsSettings
	{
		std::unordered_map<eAction, Key> ActionToKey = {{eAction::WalkForward, Key::W},
														{eAction::WalkBackward, Key::S},
														{eAction::StrafeLeft, Key::A},
														{eAction::StrafeRight, Key::D},
														{eAction::Jump, Key::Space},
														{eAction::Sneak, Key::LeftShift},
														{eAction::Sprint, Key::LeftControl},

														{eAction::ToggleMouseCapture, Key::KP7},
														{eAction::Pause, Key::Escape},
														{eAction::CloseMenu, Key::Escape},

														{eAction::Attack, Key::MouseButtonLeft},
														{eAction::Interact, Key::MouseButtonRight},

														{eAction::ToggleFlyMode, Key::Space}};
	};

	// ---------- Serialization ----------

	template <typename BasicJsonType> inline void to_json(BasicJsonType& j, const KeyBindsSettings& s)
	{
		j = BasicJsonType::object();

		// Copy to vector for sorting
		std::vector<std::pair<std::string, std::string>> entries;
		entries.reserve(s.ActionToKey.size());

		for (const auto& [action, key] : s.ActionToKey)
		{
			entries.emplace_back(ActionToString(action), KeyToString(key));
		}

		// Sort by action name (stable & deterministic)
		std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

		// Insert in sorted order → preserved by ordered_json
		for (const auto& [actionStr, keyStr] : entries)
		{
			j[actionStr] = keyStr;
		}
	}

	template <typename BasicJsonType> inline void from_json(const BasicJsonType& j, KeyBindsSettings& s)
	{
		for (auto it = j.begin(); it != j.end(); ++it)
		{
			eAction action = StringToAction(it.key());
			Key key = StringToKey(it.value().get<std::string>());
			s.ActionToKey[action] = key;
		}
	}

} // namespace onion::voxel
