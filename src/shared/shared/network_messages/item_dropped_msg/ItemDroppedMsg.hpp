#pragma once

#include <cstdint>

#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>

#include <shared/network_messages/MessageHeader.hpp>

namespace onion::voxel
{
	// Sent by the client when the player drops an item (Q key).
	// The server spawns a BlockEntity at the player's head position.
	struct ItemDroppedMsg
	{
		static constexpr MessageHeader::eType StaticType = MessageHeader::eType::ItemDropped;

		uint16_t BlockId = 0;
		uint8_t Count = 0;

		template <class Archive> void serialize(Archive& ar) { ar(BlockId, Count); }
	};
} // namespace onion::voxel
