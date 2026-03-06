#pragma once

#include <glm/glm.hpp>

#include <atomic>
#include <memory>
#include <shared_mutex>
#include <vector>

#include <shared/world/chunk/Chunk.hpp>

#include "SubChunkMesh.hpp"

namespace onion::voxel
{
	class MeshBuilder;

	class ChunkMesh
	{
		friend class MeshBuilder;

		// ----- Constructor / Destructor -----
	  public:
		ChunkMesh(const glm::ivec2& chunkPosition);
		~ChunkMesh();

		// ----- Public API -----
	  public:
		void RenderOpaque();
		void RenderCutout();
		void RenderTransparent();

		// ----- Getters / Setters -----
	  public:
		glm::ivec2 GetChunkPosition() const;
		bool IsDirty() const;
		void SetDirty(bool isDirty);
		bool IsDeleteRequested() const;
		void SetDeleteRequested(bool deleteRequested);
		bool IsReadyToBeDeleted() const;

		// ----- States -----
	  private:
		std::atomic_bool m_IsDirty{true};
		std::atomic_bool m_DeleteRequested{false};
		std::atomic_bool m_IsReadyToBeDeleted{false};

		// ----- Members -----
	  protected:
		const glm::ivec2 m_ChunkPosition; // The position of the chunk that this mesh represents (in chunk coordinates)
		mutable std::shared_mutex m_MutexSubChunkMeshes; // Mutex for synchronizing access to the subchunk meshes
		std::vector<std::shared_ptr<SubChunkMesh>> m_SubChunkMeshes; // The subchunk meshes that make up this chunk mesh
	};
} // namespace onion::voxel
