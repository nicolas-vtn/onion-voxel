#pragma once

#include <cereal/archives/binary.hpp>

#include <sstream>
#include <string>

#include <shared/data_transfer_objects/DTOs/DTOs.hpp>
#include <shared/network_messages/MessageHeader.hpp>

namespace onion::voxel
{
	struct ChunkDataMsg
	{
		static constexpr MessageHeader::eType StaticType = MessageHeader::eType::ChunkData;

		ChunkDTO Chunk;

		template <class Archive> void serialize(Archive& ar) { ar(Chunk); }
	};
} // namespace onion::voxel
