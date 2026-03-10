#include "MeshBuilder.hpp"

#include <numeric>

#include <shared/utils/Stopwatch.hpp>

namespace onion::voxel
{
	MeshBuilder::MeshBuilder(std::shared_ptr<WorldManager> worldManager, std::shared_ptr<TextureAtlas> textureAtlas)
		: m_WorldManager(worldManager), m_BlockRegistry(textureAtlas), m_TextureAtlas(textureAtlas)
	{
	}

	MeshBuilder::~MeshBuilder()
	{
		m_ThreadPool.Close();
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

		for (int i = 0; i < 6; i++)
		{
			// Water is only visible when next to air
			if (block.m_BlockID == BlockId::Water && neighbors[i].m_BlockID != BlockId::Air)
			{
				visibility[i] = false;
				continue;
			}

			if (block.m_BlockID == BlockId::OakLeaves && neighbors[i].m_BlockID == BlockId::OakLeaves)
			{
				if (i == (int) BlockFace::Top || i == (int) BlockFace::Left || i == (int) BlockFace::Back)
				{
					visibility[i] = true;
					continue;
				}
				visibility[i] = false;
				continue;
			}

			// A face is visible if the neighboring block in that direction is transparent
			visibility[i] = Block::IsTransparent(block.m_BlockID) || Block::IsTransparent(neighbors[i].m_BlockID);
		}

		return visibility;
	}

	void MeshBuilder::UpdateChunkMesh(const std::shared_ptr<ChunkMesh> chunkMesh)
	{
		// First, we need to get a shared pointer to the chunk from the weak pointer in the chunk mesh
		std::shared_ptr<Chunk> chunk = chunkMesh->m_Chunk.lock();
		if (!chunk)
			return;

		Stopwatch stopwatch;
		stopwatch.Start();

		const glm::ivec2 chunkPos = chunk->GetPosition();

		const int subChunkCount = chunk->GetSubChunkCount();

		// If the chunk mesh is already being rebuilt, we should stop the existing rebuild and start a new one
		if (chunkMesh->m_IsRebuilding)
		{
			std::lock_guard lock(chunkMesh->m_MutexStopSource);
			chunkMesh->m_RebuildStopSource.request_stop();
		}

		// Acquire the lock to set the rebuilding flag and create a new stop source for this rebuild
		std::lock_guard lock(chunkMesh->m_MutexRebuilding);
		chunkMesh->StartRebuilding();
		{
			std::lock_guard stopSourceLock(chunkMesh->m_MutexStopSource);
			chunkMesh->m_RebuildStopSource = std::stop_source();
		}
		std::stop_token stopToken = chunkMesh->m_RebuildStopSource.get_token();

		// Create a new vector of SubChunkMeshes to build the new meshes into.
		std::vector<std::shared_ptr<SubChunkMesh>> newSubChunkMeshes(subChunkCount);
		{
			std::shared_lock lock(chunkMesh->m_MutexSubChunkMeshes);
			newSubChunkMeshes = chunkMesh->m_SubChunkMeshes;
		}

		// Ensure the new vector has enough subchunk meshes for the number of subchunks in the chunk
		while (newSubChunkMeshes.size() < subChunkCount)
		{
			newSubChunkMeshes.emplace_back(std::make_shared<SubChunkMesh>());
		}

		constexpr int SIZE = WorldConstants::SUBCHUNK_SIZE;

		// Gets the adjacent chunks.
		std::shared_ptr<Chunk> adjacentPosX = m_WorldManager->GetChunk(glm::ivec2(chunkPos.x + 1, chunkPos.y));
		std::shared_ptr<Chunk> adjacentNegX = m_WorldManager->GetChunk(glm::ivec2(chunkPos.x - 1, chunkPos.y));
		std::shared_ptr<Chunk> adjacentPosZ = m_WorldManager->GetChunk(glm::ivec2(chunkPos.x, chunkPos.y + 1));
		std::shared_ptr<Chunk> adjacentNegZ = m_WorldManager->GetChunk(glm::ivec2(chunkPos.x, chunkPos.y - 1));

		for (int sub = 0; sub < subChunkCount; sub++)
		{
			// If a stop has been requested for this rebuild, we should stop building the mesh
			if (stopToken.stop_requested())
			{
				return;
			}

			auto& mesh = newSubChunkMeshes[sub];
			std::unique_lock meshLock(mesh->m_Mutex);

			// If the mesh is not dirty, we can skip it
			if (!mesh->IsDirty())
			{
				newSubChunkMeshes[sub] = mesh; // Reuse the existing mesh
				continue;
			}

			for (int z = 0; z < SIZE; z++)
				for (int y = 0; y < SIZE; y++)
					for (int x = 0; x < SIZE; x++)
					{
						const glm::ivec3 localPos(x, SIZE * sub + y, z);
						Block block = chunk->GetBlock(localPos);

						if (block.m_BlockID == BlockId::Air)
							continue;

						// ------ Calculate World Position -----
						float wx = chunkPos.x * SIZE + x;
						float wy = SIZE * sub + y;
						float wz = chunkPos.y * SIZE + z;

						// ------ Get Neighboring Blocks ------
						std::array<Block, 6> neighbors;

						// Top (+y)
						if (localPos.y + 1 < subChunkCount * SIZE)
							neighbors[(int) BlockFace::Top] = chunk->GetBlock(glm::ivec3(x, localPos.y + 1, z));
						else
							neighbors[(int) BlockFace::Top] = Block(BlockId::Air); // Air block if above the world

						// Bottom (-y)
						if (localPos.y - 1 >= 0)
							neighbors[(int) BlockFace::Bottom] = chunk->GetBlock(glm::ivec3(x, localPos.y - 1, z));
						else
							neighbors[(int) BlockFace::Bottom] = Block(BlockId::Air); // Air block if below the world

						// Front (+z)
						if (z + 1 < SIZE)
							neighbors[(int) BlockFace::Front] = chunk->GetBlock(glm::ivec3(x, localPos.y, z + 1));
						else
						{
							neighbors[(int) BlockFace::Front] = adjacentPosZ
								? adjacentPosZ->GetBlock(glm::ivec3(x, localPos.y, 0))
								: Block(BlockId::Stone);
						}

						// Back (-z)
						if (z - 1 >= 0)
							neighbors[(int) BlockFace::Back] = chunk->GetBlock(glm::ivec3(x, localPos.y, z - 1));
						else
						{
							neighbors[(int) BlockFace::Back] = adjacentNegZ
								? adjacentNegZ->GetBlock(glm::ivec3(x, localPos.y, SIZE - 1))
								: Block(BlockId::Stone);
						}

						// Right (+x)
						if (x + 1 < SIZE)
							neighbors[(int) BlockFace::Right] = chunk->GetBlock(glm::ivec3(x + 1, localPos.y, z));
						else
						{
							neighbors[(int) BlockFace::Right] = adjacentPosX
								? adjacentPosX->GetBlock(glm::ivec3(0, localPos.y, z))
								: Block(BlockId::Stone);
						}

						// Left (-x)
						if (x - 1 >= 0)
							neighbors[(int) BlockFace::Left] = chunk->GetBlock(glm::ivec3(x - 1, localPos.y, z));
						else
						{
							neighbors[(int) BlockFace::Left] = adjacentNegX
								? adjacentNegX->GetBlock(glm::ivec3(SIZE - 1, localPos.y, z))
								: Block(BlockId::Stone);
						}

						// ------ Determine Face Visibility ------
						const std::array<bool, 6> faceVisible = GetFaceVisibility(block, neighbors);

						// If no faces are visible, skip this block
						if (!std::any_of(faceVisible.begin(), faceVisible.end(), [](bool v) { return v; }))
							continue;

						// ------ Get Block Textures ------
						const BlockTextures& blockTextures = m_BlockRegistry.Get(block.m_BlockID);

						// ------ Build Mesh ------
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

		// Swap the old subchunk meshes with the new ones
		{
			std::unique_lock lock(chunkMesh->m_MutexSubChunkMeshes);
			chunkMesh->m_SubChunkMeshes = std::move(newSubChunkMeshes);
		}

		chunkMesh->FinishRebuilding();

		AddChunkMeshUpdateTime(stopwatch.ElapsedMs());
		RecordExecution();
	}

	void MeshBuilder::AddChunkMeshUpdateTime(double timeMs)
	{
		std::lock_guard lock(m_ChunkMeshUpdateTimesMutex);
		m_ChunkMeshUpdateTimes_ms.push_back(timeMs);

		if (m_ChunkMeshUpdateTimes_ms.size() > m_MaxDurationsToStore)
		{
			m_ChunkMeshUpdateTimes_ms.pop_front();
		}

		double total = std::accumulate(m_ChunkMeshUpdateTimes_ms.begin(), m_ChunkMeshUpdateTimes_ms.end(), 0.0);
		m_AverageChunkMeshUpdateTime.store(total / m_ChunkMeshUpdateTimes_ms.size());
	}

	void MeshBuilder::UpdateChunkMeshAsync(const std::shared_ptr<ChunkMesh> chunkMesh)
	{
		m_ThreadPool.Dispatch([this, chunkMesh]() { UpdateChunkMesh(chunkMesh); });
	}

	double MeshBuilder::GetAverageChunkMeshUpdateTime() const
	{
		return m_AverageChunkMeshUpdateTime.load();
	}

	size_t MeshBuilder::GetChunkMeshUpdatesLastSeconds() const
	{
		std::lock_guard lock(m_ExecutionTimesMutex);
		return m_ExecutionTimes.size();
	}

	double MeshBuilder::GetChunkMeshUpdatesPerSecond() const
	{
		std::lock_guard lock(m_ExecutionTimesMutex);

		if (m_ExecutionTimes.empty())
			return 0.0;

		auto duration = std::chrono::duration<double>(m_Window).count();
		return m_ExecutionTimes.size() / duration;
	}

	size_t MeshBuilder::GetMeshBuilderThreadCount() const
	{
		return m_ThreadPool.GetPoolsCount();
	}

	void MeshBuilder::SetMeshBuilderThreadCount(size_t count)
	{
		m_ThreadPool.SetPoolsCount(count);
	}

	void MeshBuilder::RecordExecution()
	{
		auto now = std::chrono::steady_clock::now();

		std::lock_guard lock(m_ExecutionTimesMutex);

		m_ExecutionTimes.push_back(now);

		auto cutoff = now - m_Window;

		while (!m_ExecutionTimes.empty() && m_ExecutionTimes.front() < cutoff)
			m_ExecutionTimes.pop_front();
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
							  Block::RotationType rotationType)
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

		if (rotationType == Block::RotationType::Pillar)
		{
			int rotationSteps = GetRotationSteps(block, face);
			RotateUVs(uv0, uv1, uv2, uv3, rotationSteps);
		}

		// ------ TINT HANDLING ------
		glm::ivec3 tint(255);

		switch (faceTexture.tintType)
		{
			case TintType::Grass:
				tint = glm::ivec3(153, 217, 102);
				break;

			case TintType::Foliage:
				tint = glm::ivec3(128, 204, 77);
				break;

			case TintType::Water:
				tint = glm::ivec3(77, 128, 255);
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

			vert.facing = static_cast<uint8_t>(face);
			vert.occlusion = 0;

			vert.tintR = static_cast<uint8_t>(tint.r);
			vert.tintG = static_cast<uint8_t>(tint.g);
			vert.tintB = static_cast<uint8_t>(tint.b);

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
