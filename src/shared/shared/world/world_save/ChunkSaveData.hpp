#pragma once

#include <memory>
#include <vector>

#include <shared/entities/entity/Entity.hpp>
#include <shared/world/chunk/Chunk.hpp>

namespace onion::voxel
{
	struct ChunkSaveData
	{
		std::shared_ptr<Chunk> ChunkData;
		std::vector<std::shared_ptr<Entity>> Entities;
	};
} // namespace onion::voxel
