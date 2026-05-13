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
		float FOV = 70.f;
		bool ShowDebugMenus = false;
	};

	template <typename BasicJsonType> inline void to_json(BasicJsonType& j, const ControlsSettings& s)
	{
		j = BasicJsonType{{"FOV", s.FOV},
						  {"MouseSettings", s.mouseSettings},
						  {"KeyBindsSettings", s.keyBindsSettings},
						  {"ShowDebugMenus", s.ShowDebugMenus}};
	}

	template <typename BasicJsonType> inline void from_json(const BasicJsonType& j, ControlsSettings& s)
	{
		if (j.contains("MouseSettings"))
			j.at("MouseSettings").get_to(s.mouseSettings);

		if (j.contains("KeyBindsSettings"))
			j.at("KeyBindsSettings").get_to(s.keyBindsSettings);

		if (j.contains("FOV"))
			j.at("FOV").get_to(s.FOV);

		if (j.contains("ShowDebugMenus"))
			j.at("ShowDebugMenus").get_to(s.ShowDebugMenus);
	}

} // namespace onion::voxel
