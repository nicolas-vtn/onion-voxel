#pragma once

#include <glm/glm.hpp>

#include <memory>

#include <shared/world/chunk/Chunk.hpp>
#include <shared/world/world_manager/WorldManager.hpp>

#include "../block_registry/BlockRegistry.hpp"
#include "ChunkMesh.hpp"

namespace onion::voxel
{
	class MeshBuilder
	{
		// ----- Constructor / Destructor -----
	  public:
		MeshBuilder(std::shared_ptr<WorldManager> worldManager, std::shared_ptr<TextureAtlas> textureAtlas);

		// ----- Public API -----
	  public:
		std::shared_ptr<ChunkMesh> BuildChunkMesh(const std::shared_ptr<Chunk> chunk);

		// ----- Private Members -----
	  private:
		std::shared_ptr<WorldManager> m_WorldManager;
		BlockRegistry m_BlockRegistry;
		std::shared_ptr<TextureAtlas> m_TextureAtlas;

		// ----- Private Methods -----
	  private:
		static void AddFace(SubChunkMesh& mesh,
							const glm::vec3& v0,
							const glm::vec3& v1,
							const glm::vec3& v2,
							const glm::vec3& v3,
							BlockFace face,
							const Block& block,
							const FaceTexture& faceTexture,
							const TextureAtlas::AtlasEntry& uv,
							RotationType rotationType);
	};
} // namespace onion::voxel
