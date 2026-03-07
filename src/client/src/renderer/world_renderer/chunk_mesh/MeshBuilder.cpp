#include "MeshBuilder.hpp"

namespace onion::voxel
{
	MeshBuilder::MeshBuilder(std::shared_ptr<WorldManager> worldManager, std::shared_ptr<TextureAtlas> textureAtlas)
		: m_WorldManager(worldManager), m_BlockRegistry(textureAtlas), m_TextureAtlas(textureAtlas)
	{
	}

	glm::ivec3 Cross(const glm::ivec3& a, const glm::ivec3& b)
	{
		return glm::ivec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
	}

	static inline glm::ivec3 OrientationToVector(Block::Orientation o)
	{
		switch (o)
		{
			case Block::Orientation::North:
				return {0, 0, 1};
			case Block::Orientation::South:
				return {0, 0, -1};
			case Block::Orientation::East:
				return {1, 0, 0};
			case Block::Orientation::West:
				return {-1, 0, 0};
			case Block::Orientation::Up:
				return {0, 1, 0};
			case Block::Orientation::Down:
				return {0, -1, 0};
			default:
				return {0, 0, 0};
		}
	}

	static inline glm::ivec3 FaceToVector(BlockFace f)
	{
		switch (f)
		{
			case BlockFace::Front:
				return {0, 0, 1};
			case BlockFace::Back:
				return {0, 0, -1};
			case BlockFace::Right:
				return {1, 0, 0};
			case BlockFace::Left:
				return {-1, 0, 0};
			case BlockFace::Top:
				return {0, 1, 0};
			case BlockFace::Bottom:
				return {0, -1, 0};
			default:
				return {0, 0, 0};
		}
	}

	static inline BlockFace GetTextureFaceForWorldFace(BlockFace worldFace, const Block& block)
	{
		if (block.m_Facing == Block::Orientation::None || block.m_Top == Block::Orientation::None)
		{
			return worldFace;
		}

		glm::ivec3 forward = OrientationToVector(block.m_Facing);
		glm::ivec3 up = OrientationToVector(block.m_Top);
		glm::ivec3 right = Cross(forward, up);

		glm::ivec3 worldDir = FaceToVector(worldFace);

		if (worldDir == forward)
			return BlockFace::Front;
		if (worldDir == -forward)
			return BlockFace::Back;

		if (worldDir == up)
			return BlockFace::Top;
		if (worldDir == -up)
			return BlockFace::Bottom;

		if (worldDir == right)
			return BlockFace::Right;
		if (worldDir == -right)
			return BlockFace::Left;

		return BlockFace::Front;
	}

	static int GetRotationSteps(const Block& block, BlockFace worldFace)
	{
		if (block.m_Facing == Block::Orientation::None || block.m_Facing == Block::Orientation::Up ||
			block.m_Facing == Block::Orientation::Down)
		{
			return 0;
		}

		if (block.m_Facing == Block::Orientation::North || block.m_Facing == Block::Orientation::South)
		{
			if (worldFace == BlockFace::Left || worldFace == BlockFace::Right)
				return 1;
			else
				return 0;
		}

		return 1;
	}

	static inline void RotateUVs(glm::vec2& uv0, glm::vec2& uv1, glm::vec2& uv2, glm::vec2& uv3, int rotation)
	{
		rotation &= 3;

		if (rotation == 0)
			return;

		glm::vec2 t0 = uv0;
		glm::vec2 t1 = uv1;
		glm::vec2 t2 = uv2;
		glm::vec2 t3 = uv3;

		switch (rotation)
		{
			case 1: // 90
				uv0 = t3;
				uv1 = t0;
				uv2 = t1;
				uv3 = t2;
				break;

			case 2: // 180
				uv0 = t2;
				uv1 = t3;
				uv2 = t0;
				uv3 = t1;
				break;

			case 3: // 270
				uv0 = t1;
				uv1 = t2;
				uv2 = t3;
				uv3 = t0;
				break;
		}
	}

	std::array<bool, 6> GetFaceVisibility(const Block& block, const std::array<Block, 6>& neighbors)
	{
		std::array<bool, 6> visibility{};

		// If block is transparent, all faces are visible
		if (Block::IsTransparent(block.m_BlockID))
		{
			std::fill(visibility.begin(), visibility.end(), true);
			return visibility;
		}

		for (int i = 0; i < 6; i++)
		{
			// A face is visible if the neighboring block in that direction is transparent
			visibility[i] = Block::IsTransparent(neighbors[i].m_BlockID);
		}

		return visibility;
	}

	void MeshBuilder::UpdateChunkMesh(const std::shared_ptr<ChunkMesh> chunkMesh)
	{
		// First, we need to get a shared pointer to the chunk from the weak pointer in the chunk mesh
		std::shared_ptr<Chunk> chunk = chunkMesh->m_Chunk.lock();
		if (!chunk)
			return;

		const glm::ivec2 chunkPos = chunk->GetPosition();

		const int subChunkCount = chunk->GetSubChunkCount();

		// Ensure the chunk mesh has enough subchunk meshes for the number of subchunks in the chunk
		{
			std::unique_lock lock(chunkMesh->m_MutexSubChunkMeshes);
			while (chunkMesh->m_SubChunkMeshes.size() < subChunkCount)
			{
				chunkMesh->m_SubChunkMeshes.emplace_back(std::make_unique<SubChunkMesh>());
			}
		}

		std::shared_lock lock(chunkMesh->m_MutexSubChunkMeshes);
		constexpr int SIZE = WorldConstants::SUBCHUNK_SIZE;

		for (int sub = 0; sub < subChunkCount; sub++)
		{
			auto& mesh = chunkMesh->m_SubChunkMeshes[sub];
			std::unique_lock meshLock(mesh->m_Mutex);

			// If the mesh is not dirty, we can skip it
			if (!mesh->IsDirty())
				continue;

			for (int z = 0; z < SIZE; z++)
				for (int y = 0; y < SIZE; y++)
					for (int x = 0; x < SIZE; x++)
					{
						const glm::ivec3 localPos(x, y, z);
						Block block = chunk->GetBlock(localPos);

						if (block.m_BlockID == BlockId::Air)
							continue;

						// ------ Get Neighboring Blocks ------
						std::array<Block, 6> neighbors;

						// Top (+y)
						if (y + 1 < SIZE)
							neighbors[(int) BlockFace::Top] = chunk->GetBlock(glm::ivec3(x, y + 1, z));
						else
							neighbors[(int) BlockFace::Top] = Block(); // Air

						// Bottom (-y)
						if (y - 1 >= 0)
							neighbors[(int) BlockFace::Bottom] = chunk->GetBlock(glm::ivec3(x, y - 1, z));
						else
							neighbors[(int) BlockFace::Bottom] = Block(); // Air

						// Front (+z)
						if (z + 1 < SIZE)
							neighbors[(int) BlockFace::Front] = chunk->GetBlock(glm::ivec3(x, y, z + 1));
						else
						{
							// Asks the world manager for the block in the neighboring chunk if this block is on the edge of the chunk
							neighbors[(int) BlockFace::Front] = m_WorldManager->GetBlock(
								glm::ivec3(chunkPos.x * SIZE + x, chunkPos.y * SIZE + y, (chunkPos.y + 1) * SIZE));
						}

						// Back (-z)
						if (z - 1 >= 0)
							neighbors[(int) BlockFace::Back] = chunk->GetBlock(glm::ivec3(x, y, z - 1));
						else
						{
							// Asks the world manager for the block in the neighboring chunk if this block is on the edge of the chunk
							neighbors[(int) BlockFace::Back] = m_WorldManager->GetBlock(
								glm::ivec3(chunkPos.x * SIZE + x, chunkPos.y * SIZE + y, (chunkPos.y - 1) * SIZE));
						}

						// Right (+x)
						if (x + 1 < SIZE)
							neighbors[(int) BlockFace::Right] = chunk->GetBlock(glm::ivec3(x + 1, y, z));
						else
						{
							// Asks the world manager for the block in the neighboring chunk if this block is on the edge of the chunk
							neighbors[(int) BlockFace::Right] = m_WorldManager->GetBlock(
								glm::ivec3((chunkPos.x + 1) * SIZE, chunkPos.y * SIZE + y, chunkPos.y * SIZE + z));
						}

						// Left (-x)
						if (x - 1 >= 0)
							neighbors[(int) BlockFace::Left] = chunk->GetBlock(glm::ivec3(x - 1, y, z));
						else
						{
							// Asks the world manager for the block in the neighboring chunk if this block is on the edge of the chunk
							neighbors[(int) BlockFace::Left] = m_WorldManager->GetBlock(
								glm::ivec3((chunkPos.x - 1) * SIZE, chunkPos.y * SIZE + y, chunkPos.y * SIZE + z));
						}

						// ------ Determine Face Visibility ------
						const std::array<bool, 6> faceVisible = GetFaceVisibility(block, neighbors);

						// If no faces are visible, skip this block
						if (!std::any_of(faceVisible.begin(), faceVisible.end(), [](bool v) { return v; }))
							continue;

						const BlockTextures& blockTextures = m_BlockRegistry.Get(block.m_BlockID);

						float wx = chunkPos.x * SIZE + x;
						float wy = SIZE * sub + y;
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

							AddFace(*mesh, v0, v1, v2, v3, worldFace, block, faceTex, uv, blockTextures.rotationType);

							// Overlay pass
							const FaceTexture& overlay = blockTextures.overlay[(size_t) textureFace];

							if (overlay.texture != 0)
							{
								auto uvOverlay = m_TextureAtlas->GetAtlasEntry(overlay.texture);

								AddFace(*mesh,
										v0,
										v1,
										v2,
										v3,
										worldFace,
										block,
										overlay,
										uvOverlay,
										blockTextures.rotationType);
							}
						};

						if (faceVisible[(int) BlockFace::Top])
						{
							buildFace(BlockFace::Top,
									  GetTextureFaceForWorldFace(BlockFace::Top, block),
									  p011,
									  p111,
									  p110,
									  p010);
						}

						if (faceVisible[(int) BlockFace::Bottom])
						{
							buildFace(BlockFace::Bottom,
									  GetTextureFaceForWorldFace(BlockFace::Bottom, block),
									  p000,
									  p100,
									  p101,
									  p001);
						}

						if (faceVisible[(int) BlockFace::Front])
						{
							buildFace(BlockFace::Front,
									  GetTextureFaceForWorldFace(BlockFace::Front, block),
									  p001,
									  p101,
									  p111,
									  p011);
						}

						if (faceVisible[(int) BlockFace::Back])
						{
							buildFace(BlockFace::Back,
									  GetTextureFaceForWorldFace(BlockFace::Back, block),
									  p100,
									  p000,
									  p010,
									  p110);
						}

						if (faceVisible[(int) BlockFace::Right])
						{
							buildFace(BlockFace::Right,
									  GetTextureFaceForWorldFace(BlockFace::Right, block),
									  p101,
									  p100,
									  p110,
									  p111);
						}

						if (faceVisible[(int) BlockFace::Left])
						{
							buildFace(BlockFace::Left,
									  GetTextureFaceForWorldFace(BlockFace::Left, block),
									  p000,
									  p001,
									  p011,
									  p010);
						}
					}

			mesh->SetDirty(false);
			mesh->BuffersUpdated();
		}

		chunkMesh->m_IsDirty = false;
	}

	void MeshBuilder::AddFace(SubChunkMesh& mesh,
							  const glm::vec3& v0,
							  const glm::vec3& v1,
							  const glm::vec3& v2,
							  const glm::vec3& v3,
							  BlockFace face,
							  const Block& block,
							  const FaceTexture& faceTexture,
							  const TextureAtlas::AtlasEntry& uv,
							  RotationType rotationType)
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

		// ------ ROTATE UVs ------

		glm::vec2 uv0{uv.uvMin.x, uv.uvMin.y};
		glm::vec2 uv1{uv.uvMax.x, uv.uvMin.y};
		glm::vec2 uv2{uv.uvMax.x, uv.uvMax.y};
		glm::vec2 uv3{uv.uvMin.x, uv.uvMax.y};

		if (rotationType == RotationType::Pillar)
		{
			int rotationSteps = GetRotationSteps(block, face);
			RotateUVs(uv0, uv1, uv2, uv3, rotationSteps);
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
		vertices->push_back(makeVertex(v0, uv0));
		vertices->push_back(makeVertex(v1, uv1));
		vertices->push_back(makeVertex(v2, uv2));
		vertices->push_back(makeVertex(v3, uv3));

		// ------ Add indices -----
		indices->push_back(startIndex + 0);
		indices->push_back(startIndex + 1);
		indices->push_back(startIndex + 2);

		indices->push_back(startIndex + 2);
		indices->push_back(startIndex + 3);
		indices->push_back(startIndex + 0);
	}
} // namespace onion::voxel
