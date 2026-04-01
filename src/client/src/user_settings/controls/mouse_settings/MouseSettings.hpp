#pragma once

#include <nlohmann/json.hpp>

namespace onion::voxel
{
	struct MouseSettings
	{
		float Sensitivity = 0.1f;
		float ScrollSensitivity = 1.0f;
	};

	template <typename BasicJsonType> inline void to_json(BasicJsonType& j, const MouseSettings& s)
	{
		j = BasicJsonType{{"Sensitivity", s.Sensitivity}, {"ScrollSensitivity", s.ScrollSensitivity}};
	}

	template <typename BasicJsonType> inline void from_json(const BasicJsonType& j, MouseSettings& s)
	{
		if (j.contains("Sensitivity"))
			j.at("Sensitivity").get_to(s.Sensitivity);

		if (j.contains("ScrollSensitivity"))
			j.at("ScrollSensitivity").get_to(s.ScrollSensitivity);
	}

} // namespace onion::voxel
