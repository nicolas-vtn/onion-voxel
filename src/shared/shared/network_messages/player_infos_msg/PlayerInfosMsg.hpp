#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>

#include <sstream>
#include <string>

#include <shared/data_transfer_objects/DTOs/DTOs.hpp>
#include <shared/network_messages/MessageHeader.hpp>

namespace onion::voxel
{
	struct PlayerInfoMsg
	{
		static constexpr MessageHeader::eType StaticType = MessageHeader::eType::PlayerInfos;

		PlayerDTO player;

		template <class Archive> void serialize(Archive& ar) { ar(player); }
	};
} // namespace onion::voxel
