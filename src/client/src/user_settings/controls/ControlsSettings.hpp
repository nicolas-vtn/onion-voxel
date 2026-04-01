#pragma once

#include <nlohmann/json.hpp>

#include "key_binds_settings/KeyBindsSettings.hpp"
#include "mouse_settings/MouseSettings.hpp"

namespace onion::voxel
{
	struct ControlsSettings
	{
		MouseSettings mouseSettings{};
		KeyBindsSettings keyBindsSettings{};
	};

	template <typename BasicJsonType> inline void to_json(BasicJsonType& j, const ControlsSettings& s)
	{
		j = BasicJsonType{{"MouseSettings", s.mouseSettings}, {"KeyBindsSettings", s.keyBindsSettings}};
	}

	template <typename BasicJsonType> inline void from_json(const BasicJsonType& j, ControlsSettings& s)
	{
		if (j.contains("MouseSettings"))
			j.at("MouseSettings").get_to(s.mouseSettings);

		if (j.contains("KeyBindsSettings"))
			j.at("KeyBindsSettings").get_to(s.keyBindsSettings);
	}

} // namespace onion::voxel
