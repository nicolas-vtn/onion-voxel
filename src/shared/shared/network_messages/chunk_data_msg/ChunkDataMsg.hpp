#pragma once

#include <cereal/archives/binary.hpp>

#include <sstream>
#include <string>

#include "../DTOs/DTOs.hpp"
#include "../MessageHeader.hpp"

#include <shared/network_messages/GlmSerialization.hpp>

namespace onion::voxel
{
	struct ChunkDataMsg
	{
		static constexpr MessageHeader::eType StaticType = MessageHeader::eType::ChunkData;

		glm::ivec2 Position;

		std::vector<SubChunkDTO> SubChunks;
		std::vector<BlockDTO> Palette;

		template <class Archive> void serialize(Archive& ar) { ar(Position, SubChunks, Palette); }
	};
} // namespace onion::voxel
