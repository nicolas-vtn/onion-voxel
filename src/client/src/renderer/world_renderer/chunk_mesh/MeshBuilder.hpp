#pragma once

#include <glm/glm.hpp>

#include <shared/world/chunk/Chunk.hpp>

#include "ChunkMesh.hpp"

namespace onion::voxel
{
	class MeshBuilder
	{
	  public:
		static std::shared_ptr<ChunkMesh> BuildChunkMesh(const std::shared_ptr<Chunk> chunk)
		{
			return std::make_shared<ChunkMesh>(chunk->GetPosition());
		}
	};
} // namespace onion::voxel
