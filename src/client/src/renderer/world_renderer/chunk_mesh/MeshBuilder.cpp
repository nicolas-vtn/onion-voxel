#include "MeshBuilder.hpp"

namespace onion::voxel
{
	static BlockFace FacingToBlockFace(SubChunkMesh::Facing facing)
	{
		switch (facing)
		{
			case SubChunkMesh::Facing::Top:
				return BlockFace::Top;
			case SubChunkMesh::Facing::Bottom:
				return BlockFace::Bottom;
			case SubChunkMesh::Facing::North:
				return BlockFace::Front;
			case SubChunkMesh::Facing::South:
				return BlockFace::Back;
			case SubChunkMesh::Facing::East:
				return BlockFace::Right;
			case SubChunkMesh::Facing::West:
				return BlockFace::Left;
		}

		return BlockFace::Top;
	}

	MeshBuilder::MeshBuilder(std::shared_ptr<TextureAtlas> textureAtlas)
		: m_BlockRegistry(textureAtlas), m_TextureAtlas(textureAtlas)
	{
	}

	std::shared_ptr<ChunkMesh> MeshBuilder::BuildChunkMesh(const std::shared_ptr<Chunk> chunk)
	{
		const glm::ivec2 chunkPos = chunk->GetPosition();
		std::shared_ptr<ChunkMesh> chunkMesh = std::make_shared<ChunkMesh>(chunkPos);

		if (!chunk)
			return nullptr;

		std::unique_lock lock(chunkMesh->m_MutexSubChunkMeshes);

		chunkMesh->m_SubChunkMeshes.clear();

		int subChunkCount = chunk->GetSubChunkCount();

		chunkMesh->m_SubChunkMeshes.resize(subChunkCount);
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

						const BlockTextures& blockTextures = m_BlockRegistry.Get(block.m_BlockID);

						float wx = chunkPos.x * WorldConstants::SUBCHUNK_SIZE + x;
						float wy = y;
						float wz = chunkPos.y * WorldConstants::SUBCHUNK_SIZE + z;

						glm::vec3 p000(wx, wy, wz);
						glm::vec3 p001(wx, wy, wz + 1);
						glm::vec3 p010(wx, wy + 1, wz);
						glm::vec3 p011(wx, wy + 1, wz + 1);

						glm::vec3 p100(wx + 1, wy, wz);
						glm::vec3 p101(wx + 1, wy, wz + 1);
						glm::vec3 p110(wx + 1, wy + 1, wz);
						glm::vec3 p111(wx + 1, wy + 1, wz + 1);

						// Top
						BlockFace topFace = FacingToBlockFace(SubChunkMesh::Facing::Top);
						const FaceTexture& topFaceTex = blockTextures.faces[(size_t) topFace];
						auto uv = m_TextureAtlas->GetAtlasEntry(topFaceTex.texture);
						AddFace(vertices, indices, p011, p111, p110, p010, (float) SubChunkMesh::Facing::Top, uv);

						// Bottom
						BlockFace bottomFace = FacingToBlockFace(SubChunkMesh::Facing::Bottom);
						const FaceTexture& bottomFaceTex = blockTextures.faces[(size_t) bottomFace];
						uv = m_TextureAtlas->GetAtlasEntry(bottomFaceTex.texture);
						AddFace(vertices, indices, p000, p100, p101, p001, (float) SubChunkMesh::Facing::Bottom, uv);

						// North
						BlockFace northFace = FacingToBlockFace(SubChunkMesh::Facing::North);
						const FaceTexture& northFaceTex = blockTextures.faces[(size_t) northFace];
						uv = m_TextureAtlas->GetAtlasEntry(northFaceTex.texture);
						AddFace(vertices, indices, p001, p101, p111, p011, (float) SubChunkMesh::Facing::North, uv);

						// South
						BlockFace southFace = FacingToBlockFace(SubChunkMesh::Facing::South);
						const FaceTexture& southFaceTex = blockTextures.faces[(size_t) southFace];
						uv = m_TextureAtlas->GetAtlasEntry(southFaceTex.texture);
						AddFace(vertices, indices, p100, p000, p010, p110, (float) SubChunkMesh::Facing::South, uv);

						// East
						BlockFace eastFace = FacingToBlockFace(SubChunkMesh::Facing::East);
						const FaceTexture& eastFaceTex = blockTextures.faces[(size_t) eastFace];
						uv = m_TextureAtlas->GetAtlasEntry(eastFaceTex.texture);
						AddFace(vertices, indices, p101, p100, p110, p111, (float) SubChunkMesh::Facing::East, uv);

						// West
						BlockFace westFace = FacingToBlockFace(SubChunkMesh::Facing::West);
						const FaceTexture& westFaceTex = blockTextures.faces[(size_t) westFace];
						uv = m_TextureAtlas->GetAtlasEntry(westFaceTex.texture);
						AddFace(vertices, indices, p000, p001, p011, p010, (float) SubChunkMesh::Facing::West, uv);
					}

			mesh->m_IndicesOpaqueCount = indices.size();
			mesh->SetDirty(false);

			chunkMesh->m_SubChunkMeshes[sub] = mesh;
		}

		chunkMesh->m_IsDirty = false;

		return chunkMesh;
	}

	void MeshBuilder::AddFace(std::vector<SubChunkMesh::Vertex>& vertices,
							  std::vector<uint16_t>& indices,
							  const glm::vec3& v0,
							  const glm::vec3& v1,
							  const glm::vec3& v2,
							  const glm::vec3& v3,
							  float facing,
							  const TextureAtlas::AtlasEntry& uv)
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

		vertices.push_back(makeVertex(v0, uv.uvMin.x, uv.uvMin.y));
		vertices.push_back(makeVertex(v1, uv.uvMax.x, uv.uvMin.y));
		vertices.push_back(makeVertex(v2, uv.uvMax.x, uv.uvMax.y));
		vertices.push_back(makeVertex(v3, uv.uvMin.x, uv.uvMax.y));

		indices.push_back(startIndex + 0);
		indices.push_back(startIndex + 1);
		indices.push_back(startIndex + 2);

		indices.push_back(startIndex + 2);
		indices.push_back(startIndex + 3);
		indices.push_back(startIndex + 0);
	}
} // namespace onion::voxel
