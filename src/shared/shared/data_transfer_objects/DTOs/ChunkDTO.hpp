#pragma once

#include <cereal/archives/binary.hpp>

#include <sstream>
#include <string>

#include <shared/data_transfer_objects/Serializer/GlmSerialization.hpp>

#include "BlockStateDTO.hpp"
#include "SubChunkDTO.hpp"

namespace onion::voxel
{
	struct ChunkDTO
	{
		glm::ivec2 Position{};

		std::vector<SubChunkDTO> SubChunks;
		std::vector<BlockStateDTO> Palette;

		template <class Archive> void serialize(Archive& ar) { ar(Position, SubChunks, Palette); }
	};
} // namespace onion::voxel
