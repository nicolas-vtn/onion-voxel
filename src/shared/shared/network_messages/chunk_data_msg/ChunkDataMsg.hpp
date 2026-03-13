#pragma once

#include <cereal/archives/binary.hpp>

#include <sstream>
#include <string>

#include <shared/network_messages/DTOs/DTOs.hpp>
#include <shared/network_messages/GlmSerialization.hpp>
#include <shared/network_messages/MessageHeader.hpp>

namespace onion::voxel
{
	struct ChunkDataMsg
	{
		static constexpr MessageHeader::eType StaticType = MessageHeader::eType::ChunkData;

		glm::ivec2 Position{};

		std::vector<SubChunkDTO> SubChunks;
		std::vector<BlockStateDTO> Palette;

		template <class Archive> void serialize(Archive& ar) { ar(Position, SubChunks, Palette); }
	};
} // namespace onion::voxel
