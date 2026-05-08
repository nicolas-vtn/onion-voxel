#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>

#include "ChunkDTO.hpp"
#include "EntityDTO.hpp"

namespace onion::voxel
{
	struct ChunkSaveDataDTO
	{
		ChunkDTO Chunk;
		std::vector<EntityDTO> Entities;

		template <class Archive> void serialize(Archive& ar) { ar(Chunk, Entities); }
	};
} // namespace onion::voxel
