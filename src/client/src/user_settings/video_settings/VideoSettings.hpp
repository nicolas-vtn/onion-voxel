#pragma once

#include <nlohmann/json.hpp>

namespace onion::voxel
{
	struct VideoSettings
	{
		uint32_t MaxFPS = 60;
		bool VSyncEnabled = true;

		uint8_t RenderDistance = 4;
		uint8_t SimulationDistance = 4;

		bool WailaEnabled = true;
	};

	template <typename BasicJsonType> inline void to_json(BasicJsonType& j, const VideoSettings& s)
	{
		j = BasicJsonType{{"MaxFPS", s.MaxFPS},
						  {"VSyncEnabled", s.VSyncEnabled},
						  {"RenderDistance", s.RenderDistance},
						  {"SimulationDistance", s.SimulationDistance},
						  {"WailaEnabled", s.WailaEnabled}};
	}

	template <typename BasicJsonType> inline void from_json(const BasicJsonType& j, VideoSettings& s)
	{
		if (j.contains("MaxFPS"))
			j.at("MaxFPS").get_to(s.MaxFPS);

		if (j.contains("VSyncEnabled"))
			j.at("VSyncEnabled").get_to(s.VSyncEnabled);

		if (j.contains("RenderDistance"))
			j.at("RenderDistance").get_to(s.RenderDistance);

		if (j.contains("SimulationDistance"))
			j.at("SimulationDistance").get_to(s.SimulationDistance);

		if (j.contains("WailaEnabled"))
			j.at("WailaEnabled").get_to(s.WailaEnabled);
	}

} // namespace onion::voxel
