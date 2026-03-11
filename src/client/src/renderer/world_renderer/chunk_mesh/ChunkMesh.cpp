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
			m_SubChunkMeshes.emplace_back(std::make_shared<SubChunkMesh>());
		}
	}

	ChunkMesh::~ChunkMesh() {}

	void ChunkMesh::RenderOpaque()
	{
		const glm::ivec2 chunkPos = GetChunkPosition();

		SubChunkMesh::s_Shader.setIVec2(
			"u_ChunkOffset", {chunkPos.x * WorldConstants::CHUNK_SIZE, chunkPos.y * WorldConstants::CHUNK_SIZE});

		std::shared_lock lock(m_MutexSubChunkMeshes);
		for (const auto& subChunkMesh : m_SubChunkMeshes)
		{
			subChunkMesh->RenderOpaque();
		}
	}

	void ChunkMesh::RenderCutout()
	{
		const glm::ivec2 chunkPos = GetChunkPosition();

		SubChunkMesh::s_Shader.setIVec2(
			"u_ChunkOffset", {chunkPos.x * WorldConstants::CHUNK_SIZE, chunkPos.y * WorldConstants::CHUNK_SIZE});

		std::shared_lock lock(m_MutexSubChunkMeshes);
		for (const auto& subChunkMesh : m_SubChunkMeshes)
		{
			subChunkMesh->RenderCutout();
		}
	}

	void ChunkMesh::RenderTransparent()
	{
		const glm::ivec2 chunkPos = GetChunkPosition();

		SubChunkMesh::s_Shader.setIVec2(
			"u_ChunkOffset", {chunkPos.x * WorldConstants::CHUNK_SIZE, chunkPos.y * WorldConstants::CHUNK_SIZE});

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

	void ChunkMesh::StartRebuilding()
	{
		m_IsDirty = false;
		m_IsRebuilding = true;
	}

	void ChunkMesh::FinishRebuilding()
	{
		m_IsRebuilding = false;
	}

	void ChunkMesh::ChangeChunk(std::shared_ptr<Chunk> newChunk)
	{
		m_Chunk = newChunk;
	}

	const glm::ivec2& ChunkMesh::GetChunkPosition() const
	{
		return m_ChunkPosition;
	}

	uint64_t ChunkMesh::GetVertexCount() const
	{
		std::shared_lock lock(m_MutexSubChunkMeshes);
		uint64_t count = 0;
		for (const auto& subChunkMesh : m_SubChunkMeshes)
		{
			if (subChunkMesh)
			{
				count += subChunkMesh->GetVertexCount();
			}
		}
		return count;
	}

	bool ChunkMesh::IsDirty() const
	{
		return m_IsDirty;
	}

	void ChunkMesh::SetSubChunkMeshDirty(int subChunkIndex, bool isDirty)
	{
		assert(subChunkIndex >= 0 && "SubChunk index cannot be negative");

		// If the subchunk index is out of range, we need to add new subchunk meshes until we have enough subchunk meshes to fit the index
		if (subChunkIndex >= m_SubChunkMeshes.size())
		{
			std::unique_lock lock(m_MutexSubChunkMeshes);
			while (m_SubChunkMeshes.size() <= subChunkIndex)
			{
				m_SubChunkMeshes.emplace_back(std::make_shared<SubChunkMesh>());
			}
		}

		std::shared_lock lock(m_MutexSubChunkMeshes);
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
