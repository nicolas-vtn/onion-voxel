#include "ChunkMesh.hpp"

namespace onion::voxel
{
	ChunkMesh::ChunkMesh(const glm::ivec2& chunkPosition) : m_ChunkPosition(chunkPosition) {}

	ChunkMesh::~ChunkMesh() {}

	void ChunkMesh::RenderOpaque()
	{
		std::shared_lock lock(m_MutexSubChunkMeshes);
		for (const auto& subChunkMesh : m_SubChunkMeshes)
		{
			subChunkMesh->RenderOpaque();
		}
	}

	void ChunkMesh::RenderCutout()
	{
		std::shared_lock lock(m_MutexSubChunkMeshes);
		for (const auto& subChunkMesh : m_SubChunkMeshes)
		{
			subChunkMesh->RenderCutout();
		}
	}

	void ChunkMesh::RenderTransparent()
	{
		std::shared_lock lock(m_MutexSubChunkMeshes);
		for (const auto& subChunkMesh : m_SubChunkMeshes)
		{
			subChunkMesh->RenderTransparent();
		}
	}

	glm::ivec2 ChunkMesh::GetChunkPosition() const
	{
		return m_ChunkPosition;
	}

	bool ChunkMesh::IsDirty() const
	{
		return m_IsDirty.load();
	}

	void ChunkMesh::SetDirty(bool isDirty)
	{
		m_IsDirty.store(isDirty);
	}

	bool ChunkMesh::IsDeleteRequested() const
	{
		return m_DeleteRequested;
	}

	void ChunkMesh::SetDeleteRequested(bool deleteRequested)
	{
		m_DeleteRequested.store(deleteRequested);
	}

	bool ChunkMesh::IsReadyToBeDeleted() const
	{
		return m_IsReadyToBeDeleted.load();
	}

} // namespace onion::voxel
