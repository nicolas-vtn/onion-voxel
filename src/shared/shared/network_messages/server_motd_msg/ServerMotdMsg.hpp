#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>

#include <sstream>
#include <string>

#include <shared/network_messages/MessageHeader.hpp>

namespace onion::voxel
{
	struct ServerMotdMsg
	{
		static constexpr MessageHeader::eType StaticType = MessageHeader::eType::ServerMOTD;

		std::string ServerMotd;

		int PlayerCount = 0;
		int MaxPlayers = 20;

		std::vector<std::string> PlayerNames;

		std::vector<uint8_t> ServerIconPngData;

		template <class Archive> void serialize(Archive& ar)
		{
			ar(ServerMotd, PlayerCount, MaxPlayers, PlayerNames, ServerIconPngData);
		}
	};
} // namespace onion::voxel
