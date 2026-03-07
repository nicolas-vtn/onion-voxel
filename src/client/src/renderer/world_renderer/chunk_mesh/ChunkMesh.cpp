#include "ChunkMesh.hpp"

namespace onion::voxel
{
	ChunkMesh::ChunkMesh(const glm::ivec2& chunkPosition, std::shared_ptr<Chunk> chunk)
		: m_ChunkPosition(chunkPosition), m_Chunk(chunk)
	{
		// Create as many subchunk meshes as there are subchunks in the chunk
		std::unique_lock lock(m_MutexSubChunkMeshes);
		const int subChunkCount = chunk->GetSubChunkCount();
		m_SubChunkMeshes.reserve(subChunkCount);
		for (int i = 0; i < subChunkCount; ++i)
		{
			m_SubChunkMeshes.emplace_back(std::make_unique<SubChunkMesh>());
		}
	}

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

	void ChunkMesh::Delete()
	{
		std::shared_lock lock(m_MutexSubChunkMeshes);
		for (const auto& subChunkMesh : m_SubChunkMeshes)
		{
			if (subChunkMesh)
				subChunkMesh->Delete();
		}
	}

	const glm::ivec2& ChunkMesh::GetChunkPosition() const
	{
		return m_ChunkPosition;
	}

	bool ChunkMesh::IsDirty() const
	{
		return m_IsDirty;
	}

	void ChunkMesh::SetSubChunkMeshDirty(int subChunkIndex, bool isDirty)
	{
		std::shared_lock lock(m_MutexSubChunkMeshes);

		if (subChunkIndex < 0 || subChunkIndex >= m_SubChunkMeshes.size())
			throw std::out_of_range("SubChunk index out of range");

		if (m_SubChunkMeshes[subChunkIndex])
		{
			m_SubChunkMeshes[subChunkIndex]->SetDirty(isDirty);
			if (isDirty)
				m_IsDirty = true;
		}
	}

	void ChunkMesh::SetAllSubChunkMeshesDirty(bool isDirty)
	{
		std::shared_lock lock(m_MutexSubChunkMeshes);
		for (const auto& subChunkMesh : m_SubChunkMeshes)
		{
			if (subChunkMesh)
				subChunkMesh->SetDirty(isDirty);
		}

		if (isDirty)
			m_IsDirty = true;
	}

} // namespace onion::voxel
