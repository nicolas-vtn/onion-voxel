#pragma once

#include <fstream>
#include <string>

#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

namespace onion::voxel
{
	struct ServerInfos
	{
		std::string Name;
		std::string Description;
		std::string Address;
		int PlayerCount{0};
		int MaxPlayerCount{0};
		int Ping{-1};
		uint16_t Port{0};
		std::vector<uint8_t> IconPngData;
		std::vector<std::string> PlayerNames;

		template <class Archive> void serialize(Archive& ar)
		{
			ar(Name, Description, Address, PlayerCount, MaxPlayerCount, Ping, Port, IconPngData, PlayerNames);
		}
	};

	inline void SaveServers(const std::vector<ServerInfos>& servers, const std::string& path)
	{
		std::ofstream os(path, std::ios::binary);
		cereal::BinaryOutputArchive archive(os);

		archive(servers);
	}

	inline void LoadServers(std::vector<ServerInfos>& servers, const std::string& path)
	{
		std::ifstream is(path, std::ios::binary);
		if (!is)
			return; // or handle error

		cereal::BinaryInputArchive archive(is);

		archive(servers);
	}

} // namespace onion::voxel
