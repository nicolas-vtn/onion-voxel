#include "ChunkMesh.hpp"

namespace onion::voxel
{
	ChunkMesh::ChunkMesh(const glm::ivec2& chunkPosition) : m_ChunkPosition(chunkPosition) {}

	ChunkMesh::ChunkMesh(std::shared_ptr<Chunk> chunk) : ChunkMesh(chunk->GetPosition())
	{
		BuildMesh(chunk);
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

	void ChunkMesh::BuildMesh(std::shared_ptr<Chunk> chunk)
	{
		if (!chunk)
			return;

		std::unique_lock lock(m_MutexSubChunkMeshes);

		m_SubChunkMeshes.clear();

		int subChunkCount = chunk->GetSubChunkCount();

		m_SubChunkMeshes.resize(subChunkCount);

		for (int sub = 0; sub < subChunkCount; ++sub)
		{
			auto mesh = std::make_shared<SubChunkMesh>();

			auto& vertices = mesh->m_VerticesOpaque;
			auto& indices = mesh->m_IndicesOpaque;

			for (int z = 0; z < WorldConstants::SUBCHUNK_SIZE; ++z)
				for (int y = 0; y < WorldConstants::SUBCHUNK_SIZE; ++y)
					for (int x = 0; x < WorldConstants::SUBCHUNK_SIZE; ++x)
					{
						glm::ivec3 localPos(x, y, z);
						Block block = chunk->GetBlock(localPos);

						if (block.m_BlockID == BlockId::Air)
							continue;

						float wx = m_ChunkPosition.x * WorldConstants::SUBCHUNK_SIZE + x;
						float wy = y;
						float wz = m_ChunkPosition.y * WorldConstants::SUBCHUNK_SIZE + z;

						glm::vec3 p000(wx, wy, wz);
						glm::vec3 p001(wx, wy, wz + 1);
						glm::vec3 p010(wx, wy + 1, wz);
						glm::vec3 p011(wx, wy + 1, wz + 1);

						glm::vec3 p100(wx + 1, wy, wz);
						glm::vec3 p101(wx + 1, wy, wz + 1);
						glm::vec3 p110(wx + 1, wy + 1, wz);
						glm::vec3 p111(wx + 1, wy + 1, wz + 1);

						// Top
						AddFace(vertices, indices, p011, p111, p110, p010, (float) SubChunkMesh::Facing::Top);

						// Bottom
						AddFace(vertices, indices, p000, p100, p101, p001, (float) SubChunkMesh::Facing::Bottom);

						// North
						AddFace(vertices, indices, p000, p001, p011, p010, (float) SubChunkMesh::Facing::North);

						// South
						AddFace(vertices, indices, p100, p110, p111, p101, (float) SubChunkMesh::Facing::South);

						// East
						AddFace(vertices, indices, p001, p101, p111, p011, (float) SubChunkMesh::Facing::East);

						// West
						AddFace(vertices, indices, p000, p010, p110, p100, (float) SubChunkMesh::Facing::West);
					}

			mesh->m_IndicesOpaqueCount = indices.size();
			mesh->SetDirty(false);

			m_SubChunkMeshes[sub] = mesh;
		}

		m_IsDirty = false;
	}

	void ChunkMesh::AddFace(std::vector<SubChunkMesh::Vertex>& vertices,
							std::vector<uint16_t>& indices,
							const glm::vec3& v0,
							const glm::vec3& v1,
							const glm::vec3& v2,
							const glm::vec3& v3,
							float facing)
	{
		uint16_t startIndex = static_cast<uint16_t>(vertices.size());

		auto makeVertex = [&](const glm::vec3& p, float u, float v)
		{
			SubChunkMesh::Vertex vert;
			vert.x = p.x;
			vert.y = p.y;
			vert.z = p.z;

			vert.texX = u;
			vert.texY = v;

			vert.facing = facing;
			vert.occlusion = 0.0f;

			vert.tintR = 1.0f;
			vert.tintG = 1.0f;
			vert.tintB = 1.0f;

			return vert;
		};

		vertices.push_back(makeVertex(v0, 0, 0));
		vertices.push_back(makeVertex(v1, 1, 0));
		vertices.push_back(makeVertex(v2, 1, 1));
		vertices.push_back(makeVertex(v3, 0, 1));

		indices.push_back(startIndex + 0);
		indices.push_back(startIndex + 1);
		indices.push_back(startIndex + 2);

		indices.push_back(startIndex + 2);
		indices.push_back(startIndex + 3);
		indices.push_back(startIndex + 0);
	}
} // namespace onion::voxel
