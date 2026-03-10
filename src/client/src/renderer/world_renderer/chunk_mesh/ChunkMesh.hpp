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
		ChunkMesh(const glm::ivec2& chunkPosition, std::shared_ptr<Chunk> chunk);
		~ChunkMesh();

		// ----- Public API -----
	  public:
		void RenderOpaque();
		void RenderCutout();
		void RenderTransparent();

		void Delete();

		void StartRebuilding();
		void FinishRebuilding();

		// ----- Getters / Setters -----
	  public:
		const glm::ivec2& GetChunkPosition() const;
		uint64_t GetVertexCount() const;

		bool IsDirty() const;
		void SetSubChunkMeshDirty(int subChunkIndex, bool isDirty);
		void SetAllSubChunkMeshesDirty(bool isDirty);

		// ----- States -----
	  private:
		std::atomic_bool m_IsDirty{true};
		std::atomic_bool m_IsRebuilding{false};
		std::mutex m_MutexRebuilding;
		std::mutex m_MutexStopSource;
		std::stop_source m_RebuildStopSource;

		// ----- Members -----
	  protected:
		const glm::ivec2 m_ChunkPosition; // The position of the chunk that this mesh represents (in chunk coordinates)
		mutable std::shared_mutex m_MutexSubChunkMeshes; // Mutex for synchronizing access to the subchunk meshes vector
		std::vector<std::shared_ptr<SubChunkMesh>> m_SubChunkMeshes; // The subchunk meshes that make up this chunk mesh

		std::weak_ptr<Chunk> m_Chunk; // Weak pointer to the chunk that this mesh represents
	};
} // namespace onion::voxel
