#pragma once

#include <nlohmann/json.hpp>

namespace onion::voxel
{
	struct QualityAndPerfSettings
	{
		uint8_t RenderDistance = 8;
		uint8_t SimulationDistance = 8;
	};

	template <typename BasicJsonType> inline void to_json(BasicJsonType& j, const QualityAndPerfSettings& s)
	{
		j = BasicJsonType{{"RenderDistance", s.RenderDistance}, {"SimulationDistance", s.SimulationDistance}};
	}

	template <typename BasicJsonType> inline void from_json(const BasicJsonType& j, QualityAndPerfSettings& s)
	{
		if (j.contains("RenderDistance"))
			j.at("RenderDistance").get_to(s.RenderDistance);

		if (j.contains("SimulationDistance"))
			j.at("SimulationDistance").get_to(s.SimulationDistance);
	}

} // namespace onion::voxel
