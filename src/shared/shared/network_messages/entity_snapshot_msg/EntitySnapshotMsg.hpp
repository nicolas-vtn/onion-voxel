#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>

#include <sstream>
#include <string>

#include <shared/network_messages/DTOs/EntityDTO.hpp>
#include <shared/network_messages/GlmSerialization.hpp>
#include <shared/network_messages/MessageHeader.hpp>

namespace onion::voxel
{
	struct EntitySnapshotMsg
	{
		static constexpr MessageHeader::eType StaticType = MessageHeader::eType::EntitySnapshot;

		double Timestamp = 0.0; // Time when the snapshot was taken (in seconds since epoch)

		std::vector<EntityDTO> Entities;

		template <class Archive> void serialize(Archive& ar) { ar(Timestamp, Entities); }
	};
} // namespace onion::voxel
