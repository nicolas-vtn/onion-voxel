#pragma once

#include <cstdint>

#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>

#include <shared/network_messages/MessageHeader.hpp>

namespace onion::voxel
{
	// Sent by the server to a client when a dropped item is picked up.
	// One message is sent per affected inventory slot (partial pickups may produce two messages).
	struct ItemPickedUpMsg
	{
		static constexpr MessageHeader::eType StaticType = MessageHeader::eType::ItemPickedUp;

		bool IsHotbar = false; // true = hotbar slot, false = main inventory slot
		uint8_t Index = 0;    // slot index within that inventory
		uint16_t ItemId = 0;  // BlockId cast to uint16_t
		uint8_t Count = 0;    // new total count in the slot after pickup

		template <class Archive> void serialize(Archive& ar) { ar(IsHotbar, Index, ItemId, Count); }
	};
} // namespace onion::voxel
