#pragma once

#include <glm/glm.hpp>

#include "../world/WorldConstants.hpp"

namespace onion::voxel::Utils
{
	inline int FloorDiv(int a, int b)
	{
		int q = a / b;
		int r = a % b;
		if (r < 0)
			--q;
		return q;
	}

	inline int FloorMod(int a, int b)
	{
		int r = a % b;
		if (r < 0)
			r += b;
		return r;
	}

	inline glm::ivec2 WorldToChunkPosition(const glm::ivec3& worldPosition)
	{
		return glm::ivec2(FloorDiv(worldPosition.x, WorldConstants::CHUNK_SIZE),
						  FloorDiv(worldPosition.z, WorldConstants::CHUNK_SIZE));
	}

	inline glm::ivec3 WorldToLocalPosition(const glm::ivec3& worldPosition)
	{
		return glm::ivec3(FloorMod(worldPosition.x, WorldConstants::CHUNK_SIZE),
						  worldPosition.y,
						  FloorMod(worldPosition.z, WorldConstants::CHUNK_SIZE));
	}

	inline glm::ivec3 LocalToWorldPosition(const glm::ivec3& localPosition, const glm::ivec2& chunkPosition)
	{
		return glm::ivec3(chunkPosition.x * WorldConstants::CHUNK_SIZE + localPosition.x,
						  localPosition.y,
						  chunkPosition.y * WorldConstants::CHUNK_SIZE + localPosition.z);
	}
}; // namespace onion::voxel::Utils
