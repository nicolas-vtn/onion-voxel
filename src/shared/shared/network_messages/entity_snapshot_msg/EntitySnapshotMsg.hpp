#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>

#include <sstream>
#include <string>

#include <shared/data_transfer_objects/DTOs/DTOs.hpp>
#include <shared/network_messages/MessageHeader.hpp>

namespace onion::voxel
{
	struct EntitySnapshotMsg
	{
		static constexpr MessageHeader::eType StaticType = MessageHeader::eType::EntitySnapshot;

		double Timestamp = 0.0; // Time when the snapshot was taken (in seconds since epoch)

		std::vector<PlayerDTO> Players;
		std::vector<EntityDTO> Entities;

		template <class Archive> void serialize(Archive& ar) { ar(Timestamp, Players, Entities); }
	};
} // namespace onion::voxel
