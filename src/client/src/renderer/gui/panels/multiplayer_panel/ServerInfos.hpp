#pragma once

#include <string>

namespace onion::voxel
{
	struct ServerInfos
	{
		std::string Name;
		std::string Description;
		std::string Address;
		int PlayerCount{0};
		int MaxPlayerCount{0};
		int Ping{0};
		uint16_t Port{0};
	};
} // namespace onion::voxel
