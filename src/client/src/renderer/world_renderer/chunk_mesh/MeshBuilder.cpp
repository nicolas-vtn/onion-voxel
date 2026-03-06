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

	static inline glm::ivec3 Cross(const glm::ivec3& a, const glm::ivec3& b)
	{
		return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
	}

	static inline BlockFace GetRotatedFace(BlockFace face, const Block& block)
	{
		using Orientation = Block::Orientation;

		auto OrientationToVec = [](Orientation o) -> glm::ivec3
		{
			switch (o)
			{
				case Orientation::Up:
					return {0, 1, 0};
				case Orientation::Down:
					return {0, -1, 0};
				case Orientation::North:
					return {0, 0, -1};
				case Orientation::South:
					return {0, 0, 1};
				case Orientation::East:
					return {1, 0, 0};
				case Orientation::West:
					return {-1, 0, 0};
				default:
					return {0, 0, 0};
			}
		};

		auto VecToFace = [](const glm::ivec3& v) -> BlockFace
		{
			if (v == glm::ivec3(0, 1, 0))
				return BlockFace::Top;
			if (v == glm::ivec3(0, -1, 0))
				return BlockFace::Bottom;
			if (v == glm::ivec3(0, 0, -1))
				return BlockFace::Front;
			if (v == glm::ivec3(0, 0, 1))
				return BlockFace::Back;
			if (v == glm::ivec3(-1, 0, 0))
				return BlockFace::Left;
			if (v == glm::ivec3(1, 0, 0))
				return BlockFace::Right;

			return BlockFace::Front;
		};

		const glm::ivec3 front = OrientationToVec(block.m_Facing);
		const glm::ivec3 top = OrientationToVec(block.m_Top);
		const glm::ivec3 right = Cross(front, top);

		glm::ivec3 dir;

		switch (face)
		{
			case BlockFace::Front:
				dir = front;
				break;
			case BlockFace::Back:
				dir = -front;
				break;
			case BlockFace::Top:
				dir = top;
				break;
			case BlockFace::Bottom:
				dir = -top;
				break;
			case BlockFace::Right:
				dir = right;
				break;
			case BlockFace::Left:
				dir = -right;
				break;
			default:
				return face;
		}

		return VecToFace(dir);
	}

	std::shared_ptr<ChunkMesh> MeshBuilder::BuildChunkMesh(const std::shared_ptr<Chunk> chunk)
	{
		if (!chunk)
			return nullptr;

		const glm::ivec2 chunkPos = chunk->GetPosition();
		auto chunkMesh = std::make_shared<ChunkMesh>(chunkPos);

		std::unique_lock lock(chunkMesh->m_MutexSubChunkMeshes);

		const int subChunkCount = chunk->GetSubChunkCount();
		chunkMesh->m_SubChunkMeshes.clear();
		chunkMesh->m_SubChunkMeshes.resize(subChunkCount);

		constexpr int SIZE = WorldConstants::SUBCHUNK_SIZE;

		for (int sub = 0; sub < subChunkCount; ++sub)
		{
			auto mesh = std::make_shared<SubChunkMesh>();

			for (int z = 0; z < SIZE; ++z)
				for (int y = 0; y < SIZE; ++y)
					for (int x = 0; x < SIZE; ++x)
					{
						glm::ivec3 localPos(x, y, z);
						Block block = chunk->GetBlock(localPos);

						if (block.m_BlockID == BlockId::Air)
							continue;

						const BlockTextures& blockTextures = m_BlockRegistry.Get(block.m_BlockID);

						float wx = chunkPos.x * SIZE + x;
						float wy = y;
						float wz = chunkPos.y * SIZE + z;

						glm::vec3 p000(wx, wy, wz);
						glm::vec3 p001(wx, wy, wz + 1);
						glm::vec3 p010(wx, wy + 1, wz);
						glm::vec3 p011(wx, wy + 1, wz + 1);

						glm::vec3 p100(wx + 1, wy, wz);
						glm::vec3 p101(wx + 1, wy, wz + 1);
						glm::vec3 p110(wx + 1, wy + 1, wz);
						glm::vec3 p111(wx + 1, wy + 1, wz + 1);

						auto buildFace = [&](BlockFace worldFace,
											 BlockFace textureFace,
											 const glm::vec3& v0,
											 const glm::vec3& v1,
											 const glm::vec3& v2,
											 const glm::vec3& v3)
						{
							const FaceTexture& faceTex = blockTextures.faces[(size_t) textureFace];

							auto uv = m_TextureAtlas->GetAtlasEntry(faceTex.texture);

							AddFace(*mesh, v0, v1, v2, v3, worldFace, block, faceTex, uv);
							// Optional overlay pass
							const FaceTexture& overlay = blockTextures.overlay[(size_t) textureFace];

							if (overlay.texture != 0)
							{
								auto uvOverlay = m_TextureAtlas->GetAtlasEntry(overlay.texture);

								AddFace(*mesh, v0, v1, v2, v3, worldFace, block, overlay, uvOverlay);
							}
						};

						if (block.m_IsRotated)
						{
							buildFace(BlockFace::Top, GetRotatedFace(BlockFace::Top, block), p011, p111, p110, p010);
							buildFace(
								BlockFace::Bottom, GetRotatedFace(BlockFace::Bottom, block), p000, p100, p101, p001);
							buildFace(
								BlockFace::Front, GetRotatedFace(BlockFace::Front, block), p001, p101, p111, p011);
							buildFace(BlockFace::Back, GetRotatedFace(BlockFace::Back, block), p100, p000, p010, p110);
							buildFace(
								BlockFace::Right, GetRotatedFace(BlockFace::Right, block), p101, p100, p110, p111);
							buildFace(BlockFace::Left, GetRotatedFace(BlockFace::Left, block), p000, p001, p011, p010);
						}
						else
						{
							buildFace(BlockFace::Top, BlockFace::Top, p011, p111, p110, p010);
							buildFace(BlockFace::Bottom, BlockFace::Bottom, p000, p100, p101, p001);
							buildFace(BlockFace::Front, BlockFace::Front, p001, p101, p111, p011);
							buildFace(BlockFace::Back, BlockFace::Back, p100, p000, p010, p110);
							buildFace(BlockFace::Right, BlockFace::Right, p101, p100, p110, p111);
							buildFace(BlockFace::Left, BlockFace::Left, p000, p001, p011, p010);
						}
					}

			mesh->m_IndicesOpaqueCount = mesh->m_IndicesOpaque.size();
			mesh->SetDirty(false);

			chunkMesh->m_SubChunkMeshes[sub] = mesh;
		}

		chunkMesh->m_IsDirty = false;

		return chunkMesh;
	}

	void MeshBuilder::AddFace(SubChunkMesh& mesh,
							  const glm::vec3& v0,
							  const glm::vec3& v1,
							  const glm::vec3& v2,
							  const glm::vec3& v3,
							  BlockFace face,
							  const Block& block,
							  const FaceTexture& faceTexture,
							  const TextureAtlas::AtlasEntry& uv)
	{
		std::vector<SubChunkMesh::Vertex>* vertices = nullptr;
		std::vector<uint16_t>* indices = nullptr;

		switch (faceTexture.textureType)
		{
			case TextureType::Opaque:
				vertices = &mesh.m_VerticesOpaque;
				indices = &mesh.m_IndicesOpaque;
				break;

			case TextureType::Cutout:
				vertices = &mesh.m_VerticesCutout;
				indices = &mesh.m_IndicesCutout;
				break;

			case TextureType::Transparent:
				vertices = &mesh.m_VerticesTransparent;
				indices = &mesh.m_IndicesTransparent;
				break;
		}

		// ------ TINT HANDLING ------
		glm::vec3 tint(1.0f);

		switch (faceTexture.tintType)
		{
			case TintType::Grass:
				tint = glm::vec3(0.6f, 0.85f, 0.4f);
				break;

			case TintType::Foliage:
				tint = glm::vec3(0.5f, 0.8f, 0.3f);
				break;

			case TintType::Water:
				tint = glm::vec3(0.3f, 0.5f, 1.0f);
				break;

			default:
				break;
		}

		// ------ VERTEX CREATION ------
		uint16_t startIndex = static_cast<uint16_t>(vertices->size());

		auto makeVertex = [&](const glm::vec3& p, const glm::vec2& uv)
		{
			SubChunkMesh::Vertex vert;

			vert.x = p.x;
			vert.y = p.y;
			vert.z = p.z;

			vert.texX = uv.x;
			vert.texY = uv.y;

			vert.facing = static_cast<float>(face);
			vert.occlusion = 0.0f;

			vert.tintR = tint.r;
			vert.tintG = tint.g;
			vert.tintB = tint.b;

			return vert;
		};

		// ------ Add vertices -----
		vertices->push_back(makeVertex(v0, {uv.uvMin.x, uv.uvMin.y}));
		vertices->push_back(makeVertex(v1, {uv.uvMax.x, uv.uvMin.y}));
		vertices->push_back(makeVertex(v2, {uv.uvMax.x, uv.uvMax.y}));
		vertices->push_back(makeVertex(v3, {uv.uvMin.x, uv.uvMax.y}));

		// ------ Add indices -----
		indices->push_back(startIndex + 0);
		indices->push_back(startIndex + 1);
		indices->push_back(startIndex + 2);

		indices->push_back(startIndex + 2);
		indices->push_back(startIndex + 3);
		indices->push_back(startIndex + 0);
	}
} // namespace onion::voxel
