#pragma once

#include <filesystem>

#include <glm/glm.hpp>

#include <shared/world/WorldConstants.hpp>

namespace onion::voxel::Utils
{
	inline int FloorDiv(int a, int b)
	{
		return (a >= 0) ? (a / b) : -((-a + b - 1) / b);
	}

	inline int FloorMod(int a, int b)
	{
		assert(b > 0);
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

	std::filesystem::path GetExecutableDirectory();

	std::string SanitizeFileName(const std::string& name);

	std::string GenerateUUID();

}; // namespace onion::voxel::Utils
