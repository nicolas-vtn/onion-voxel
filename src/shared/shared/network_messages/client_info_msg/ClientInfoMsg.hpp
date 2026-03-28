#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>

#include <sstream>
#include <string>

#include <shared/network_messages/MessageHeader.hpp>

namespace onion::voxel
{
	struct ClientInfoMsg
	{
		static constexpr MessageHeader::eType StaticType = MessageHeader::eType::ClientInfo;

		std::string PlayerName;
		std::string UUID;

		template <class Archive> void serialize(Archive& ar) { ar(PlayerName, UUID); }
	};
} // namespace onion::voxel
