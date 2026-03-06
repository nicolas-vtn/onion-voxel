#pragma once

#include <glm/glm.hpp>

#include <memory>

#include <shared/world/chunk/Chunk.hpp>

#include "../block_registry/BlockRegistry.hpp"
#include "ChunkMesh.hpp"

namespace onion::voxel
{
	class MeshBuilder
	{
		// ----- Constructor / Destructor -----
	  public:
		MeshBuilder(std::shared_ptr<TextureAtlas> textureAtlas);

		// ----- Public API -----
	  public:
		std::shared_ptr<ChunkMesh> BuildChunkMesh(const std::shared_ptr<Chunk> chunk);

		// ----- Private Members -----
	  private:
		BlockRegistry m_BlockRegistry;
		std::shared_ptr<TextureAtlas> m_TextureAtlas;

		// ----- Private Methods -----
	  private:
		void AddFace(std::vector<SubChunkMesh::Vertex>& vertices,
					 std::vector<uint16_t>& indices,
					 const glm::vec3& v0,
					 const glm::vec3& v1,
					 const glm::vec3& v2,
					 const glm::vec3& v3,
					 float facing,
					 const TextureAtlas::AtlasEntry& uv);
	};
} // namespace onion::voxel
