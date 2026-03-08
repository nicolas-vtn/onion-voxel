#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>

#include <sstream>
#include <string>

#include "../MessageHeader.hpp"

namespace onion::voxel
{
	struct ServerInfoMsg
	{
		static constexpr MessageHeader::eType StaticType = MessageHeader::eType::ServerInfo;

		uint32_t ClientHandle;

		std::string ServerName;
		int SimulationDistance;

		template <class Archive> void serialize(Archive& ar) { ar(ServerName, ClientHandle, SimulationDistance); }
	};
} // namespace onion::voxel
