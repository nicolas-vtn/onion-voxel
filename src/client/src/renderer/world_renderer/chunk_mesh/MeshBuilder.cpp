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

	std::array<bool, 6> GetFaceVisibility(const BlockState& block, const std::array<BlockState, 6>& neighbors)
	{
		std::array<bool, 6> visibility{true};

		for (int i = 0; i < 6; i++)
		{
			// Water is only visible when next to air
			if (block.ID == BlockId::Water && neighbors[i].ID != BlockId::Air)
			{
				visibility[i] = false;
				continue;
			}
			else if (block.ID == BlockId::Ice && neighbors[i].ID != BlockId::Air && neighbors[i].ID != BlockId::Water)
			{
				visibility[i] = false;
				continue;
			}

			//if (block.ID == BlockId::OakLeaves && neighbors[i].ID == BlockId::OakLeaves)
			//{
			//	if (i == (int) Face::Top || i == (int) Face::Left || i == (int) Face::Back)
			//	{
			//		visibility[i] = true;
			//		continue;
			//	}
			//	visibility[i] = false;
			//	continue;
			//}

			// A face is visible if the neighboring block in that direction is transparent
			visibility[i] = BlockState::IsTransparent(block.ID) || BlockState::IsTransparent(neighbors[i].ID);
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
		std::lock_guard lockRebuild(chunkMesh->m_MutexRebuilding);
		chunkMesh->StartRebuilding();
		{
			std::lock_guard stopSourceLock(chunkMesh->m_MutexStopSource);
			chunkMesh->m_RebuildStopSource = std::stop_source();
		}
		std::stop_token stopToken = chunkMesh->m_RebuildStopSource.get_token();

		// Create a new vector of SubChunkMeshes to build the new meshes into.
		std::vector<std::shared_ptr<SubChunkMesh>> newSubChunkMeshes(subChunkCount);
		{
			std::shared_lock lockSubChunkMeshes(chunkMesh->m_MutexSubChunkMeshes);
			newSubChunkMeshes = chunkMesh->m_SubChunkMeshes;
		}

		// Ensure the new vector has enough subchunk meshes for the number of subchunks in the chunk
		while (newSubChunkMeshes.size() < subChunkCount)
		{
			newSubChunkMeshes.emplace_back(std::make_shared<SubChunkMesh>());
		}

		constexpr int SIZE = WorldConstants::CHUNK_SIZE;

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

			// Build Occlusion Map
			BuildOcclusionMap(mesh, sub, chunk, adjacentPosX, adjacentNegX, adjacentPosZ, adjacentNegZ);

			for (int z = 0; z < SIZE; z++)
				for (int y = 0; y < SIZE; y++)
					for (int x = 0; x < SIZE; x++)
					{
						const glm::ivec3 localPos(x, SIZE * sub + y, z);
						BlockState block = chunk->GetBlock(localPos);

						if (block.ID == BlockId::Air)
							continue;

						// ------ Calculate World Position -----
						//int wx = chunkPos.x * SIZE + x;
						int wy = SIZE * sub + y;
						//int wz = chunkPos.y * SIZE + z;

						// ------ Get Neighboring Blocks ------
						std::array<BlockState, 6> neighbors;

						// Top (+y)
						if (localPos.y + 1 < subChunkCount * SIZE)
							neighbors[(int) Face::Top] = chunk->GetBlock(glm::ivec3(x, localPos.y + 1, z));
						else
							neighbors[(int) Face::Top] = BlockState(BlockId::Air); // Air block if above the world

						// Bottom (-y)
						if (localPos.y - 1 >= 0)
							neighbors[(int) Face::Bottom] = chunk->GetBlock(glm::ivec3(x, localPos.y - 1, z));
						else
							neighbors[(int) Face::Bottom] = BlockState(BlockId::Air); // Air block if below the world

						// Front (+z)
						if (z + 1 < SIZE)
							neighbors[(int) Face::Front] = chunk->GetBlock(glm::ivec3(x, localPos.y, z + 1));
						else
						{
							neighbors[(int) Face::Front] = adjacentPosZ
								? adjacentPosZ->GetBlock(glm::ivec3(x, localPos.y, 0))
								: BlockState(BlockId::Stone);
						}

						// Back (-z)
						if (z - 1 >= 0)
							neighbors[(int) Face::Back] = chunk->GetBlock(glm::ivec3(x, localPos.y, z - 1));
						else
						{
							neighbors[(int) Face::Back] = adjacentNegZ
								? adjacentNegZ->GetBlock(glm::ivec3(x, localPos.y, SIZE - 1))
								: BlockState(BlockId::Stone);
						}

						// Right (+x)
						if (x + 1 < SIZE)
							neighbors[(int) Face::Right] = chunk->GetBlock(glm::ivec3(x + 1, localPos.y, z));
						else
						{
							neighbors[(int) Face::Right] = adjacentPosX
								? adjacentPosX->GetBlock(glm::ivec3(0, localPos.y, z))
								: BlockState(BlockId::Stone);
						}

						// Left (-x)
						if (x - 1 >= 0)
							neighbors[(int) Face::Left] = chunk->GetBlock(glm::ivec3(x - 1, localPos.y, z));
						else
						{
							neighbors[(int) Face::Left] = adjacentNegX
								? adjacentNegX->GetBlock(glm::ivec3(SIZE - 1, localPos.y, z))
								: BlockState(BlockId::Stone);
						}

						// ------ Determine Face Visibility ------
						const std::array<bool, 6> faceVisible = GetFaceVisibility(block, neighbors);

						// If no faces are visible, skip this block
						if (!std::any_of(faceVisible.begin(), faceVisible.end(), [](bool v) { return v; }))
							continue;

						// ------ Get Block Textures ------
						const BlockTextures& blockTextures = m_BlockRegistry.Get(block.ID, block.VariantIndex);

						// ------ Build Mesh ------
						if (blockTextures.textureModel == Model::Block)
						{
							// Build each face individually using its own element geometry (from/to)
							for (size_t i = 0; i < blockTextures.faces.size(); i++)
							{
								const int& faceIdx = (int) blockTextures.faces[i].face;

								if (!faceVisible[faceIdx])
									continue;

								const FaceTexture& faceTexture = blockTextures.faces[i];

								if (faceTexture.texture == UINT16_MAX)
								{
									continue;
								}

								PointsAndOcclusion pao = GetPointsAndOcclusionForBlock(
									mesh.get(), x, wy, z, faceTexture.from, faceTexture.to);

								std::vector<FaceBuildDesc> faceDescs = GetBlockFaceBuildDescs(pao);
								// Only emit the descriptor matching this face index
								for (const auto& f : faceDescs)
								{
									if ((int) f.face != faceIdx)
										continue;
									BuildFace(*m_TextureAtlas, *mesh, blockTextures, f);
								}
							}
						}
						else
						{
							// Non-block models (Cross etc.) use a single shared pao
							PointsAndOcclusion pao = GetPointsAndOcclusion(blockTextures, mesh.get(), x, wy, z);
							std::vector<FaceBuildDesc> faces = GetFaceBuildDescs(blockTextures, pao);

							for (const auto& f : faces)
							{
								int idx = (int) f.face;

								if (!faceVisible[idx])
									continue;

								BuildFace(*m_TextureAtlas, *mesh, blockTextures, f);
							}
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

	void MeshBuilder::BuildOcclusionMap(const std::shared_ptr<SubChunkMesh> subMesh,
										const int subChunkIndex,
										const std::shared_ptr<Chunk>& chunk,
										const std::shared_ptr<Chunk>& adjacentPosX,
										const std::shared_ptr<Chunk>& adjacentNegX,
										const std::shared_ptr<Chunk>& adjacentPosZ,
										const std::shared_ptr<Chunk>& adjacentNegZ)
	{

		// No Occlusion map on air chunks
		bool isMonoBlock = chunk->IsSubchunkMonoBlock(subChunkIndex);
		if (isMonoBlock && chunk->GetBlock({0, subChunkIndex * WorldConstants::CHUNK_SIZE + 1, 0}).ID == BlockId::Air)
			return;

		constexpr int SX = WorldConstants::CHUNK_SIZE;
		constexpr int SY = WorldConstants::CHUNK_SIZE;
		constexpr int SZ = WorldConstants::CHUNK_SIZE;

		const int yMini = subChunkIndex * SY;

		// 1) Build solid masks from subchunk (fast local reads, no locks)
		using Row = uint64_t;	 // bits along X or Z or Y (16 wide)
		Row solidX[SY][SZ] = {}; // [y][z] bits along x
		Row solidZ[SY][SX] = {}; // [y][x] bits along z
		Row solidY[SZ][SX] = {}; // [z][x] bits along y

		if (isMonoBlock)
		{
			const auto mono = chunk->GetBlock({0, yMini, 0});
			const bool solid = BlockState::IsOpaque(mono.ID);
			if (solid)
			{
				const Row FULL_X = Row((1ull << SX) - 1ull);
				const Row FULL_Z = Row((1ull << SZ) - 1ull);
				const Row FULL_Y = Row((1ull << SY) - 1ull);

				for (int y = 0; y < SY; y++)
				{
					for (int z = 0; z < SZ; z++)
						solidX[y][z] = FULL_X;
					for (int x = 0; x < SX; x++)
						solidZ[y][x] = FULL_Z;
				}
				for (int z = 0; z < SZ; z++)
					for (int x = 0; x < SX; x++)
						solidY[z][x] = FULL_Y;
			}
		}
		else
		{
			for (int y = 0; y < SY; y++)
			{
				for (int z = 0; z < SZ; z++)
				{
					Row rowX = 0;
					for (int x = 0; x < SX; x++)
					{
						const BlockId id =
							chunk->GetBlock({static_cast<int>(x), static_cast<int>(y) + yMini, static_cast<int>(z)}).ID;
						const bool s = BlockState::IsOpaque(id);
						rowX |= Row(s) << x;		 // along X in [y][z]
						solidZ[y][x] |= Row(s) << z; // along Z in [y][x]
						solidY[z][x] |= Row(s) << y; // along Y in [z][x]
					}
					solidX[y][z] = rowX;
				}
			}
		}

		// 2) Load neighbor borders once
		uint8_t nbrXneg[SY][SZ] = {}, nbrXpos[SY][SZ] = {};
		uint8_t nbrZneg[SY][SX] = {}, nbrZpos[SY][SX] = {};
		uint8_t nbrYneg[SZ][SX] = {}, nbrYpos[SZ][SX] = {};

		const glm::ivec2 chunkPos = chunk->GetPosition();

		// X- (x = -1) and X+ (x = SX)
		for (int ly = 0; ly < SY; ly++)
		{
			for (int lz = 0; lz < SZ; lz++)
			{

				// X-
				{
					BlockId blockId = adjacentNegX ? adjacentNegX->GetBlock({SX - 1, ly + yMini, lz}).ID : BlockId::Air;
					nbrXneg[ly][lz] = BlockState::IsOpaque(blockId) ? 1 : 0;
				}
				// X+
				{
					BlockId blockId = adjacentPosX ? adjacentPosX->GetBlock({0, ly + yMini, lz}).ID : BlockId::Air;
					nbrXpos[ly][lz] = BlockState::IsOpaque(blockId) ? 1 : 0;
				}
			}
		}

		// Z- (z = -1) and Z+ (z = SZ)
		for (int ly = 0; ly < SY; ly++)
		{
			for (int lx = 0; lx < SX; lx++)
			{

				// Z-
				{
					BlockId blockId = adjacentNegZ ? adjacentNegZ->GetBlock({lx, ly + yMini, SZ - 1}).ID : BlockId::Air;
					nbrZneg[ly][lx] = BlockState::IsOpaque(blockId) ? 1 : 0;
				}
				// Z+
				{
					BlockId blockId = adjacentPosZ ? adjacentPosZ->GetBlock({lx, ly + yMini, 0}).ID : BlockId::Air;
					nbrZpos[ly][lx] = BlockState::IsOpaque(blockId) ? 1 : 0;
				}
			}
		}

		// Y- (y = -1) and Y+ (y = SY)
		for (int lz = 0; lz < SZ; lz++)
		{
			for (int lx = 0; lx < SX; lx++)
			{
				// Y-
				{
					BlockId blockId = chunk->GetBlock({lx, yMini - 1, lz}).ID;
					nbrYneg[lz][lx] = BlockState::IsOpaque(blockId) ? 1 : 0;
				}
				// Y+
				{
					BlockId blockId = chunk->GetBlock({lx, yMini + SY, lz}).ID;
					nbrYpos[lz][lx] = BlockState::IsOpaque(blockId) ? 1 : 0;
				}
			}
		}

		// 3) Helper
		auto solidCell = [&](int x, int y, int z) noexcept -> bool
		{
			if ((unsigned) x < SX && (unsigned) y < SY && (unsigned) z < SZ)
			{
				return (solidX[y][z] >> x) & 1;
			}
			if (x == -1 && (unsigned) y < SY && (unsigned) z < SZ)
				return nbrXneg[y][z];
			if (x == SX && (unsigned) y < SY && (unsigned) z < SZ)
				return nbrXpos[y][z];
			if (z == -1 && (unsigned) y < SY && (unsigned) x < SX)
				return nbrZneg[y][x];
			if (z == SZ && (unsigned) y < SY && (unsigned) x < SX)
				return nbrZpos[y][x];
			if (y == -1 && (unsigned) z < SZ && (unsigned) x < SX)
				return nbrYneg[z][x];
			if (y == SY && (unsigned) z < SZ && (unsigned) x < SX)
				return nbrYpos[z][x];
			return false;
		};
		auto c4 = [](bool a, bool b, bool c, bool d) { return int(a) + int(b) + int(c) + int(d); };

		// 4) Build occlusion bytes
		const int NX = SX + 1, NY = SY + 1, NZ = SZ + 1;
		std::vector<uint8_t> occMap(NX * NY * NZ, 0);

		for (int z = 0; z < NZ; z++)
		{
			for (int y = 0; y < NY; y++)
			{
				for (int x = 0; x < NX; x++)
				{
					const int cx0 = x - 1, cx1 = x;
					const int cy0 = y - 1, cy1 = y;
					const int cz0 = z - 1, cz1 = z;

					const int pY = c4(solidCell(cx0, cy1, cz0),
									  solidCell(cx1, cy1, cz0),
									  solidCell(cx0, cy1, cz1),
									  solidCell(cx1, cy1, cz1));
					const int mY = c4(solidCell(cx0, cy0, cz0),
									  solidCell(cx1, cy0, cz0),
									  solidCell(cx0, cy0, cz1),
									  solidCell(cx1, cy0, cz1));
					const int pX = c4(solidCell(cx1, cy0, cz0),
									  solidCell(cx1, cy1, cz0),
									  solidCell(cx1, cy0, cz1),
									  solidCell(cx1, cy1, cz1));
					const int mX = c4(solidCell(cx0, cy0, cz0),
									  solidCell(cx0, cy1, cz0),
									  solidCell(cx0, cy0, cz1),
									  solidCell(cx0, cy1, cz1));
					const int pZ = c4(solidCell(cx0, cy0, cz1),
									  solidCell(cx1, cy0, cz1),
									  solidCell(cx0, cy1, cz1),
									  solidCell(cx1, cy1, cz1));
					const int mZ = c4(solidCell(cx0, cy0, cz0),
									  solidCell(cx1, cy0, cz0),
									  solidCell(cx0, cy1, cz0),
									  solidCell(cx1, cy1, cz0));

					const int minCnt = std::min(std::min(pY, mY), std::min(std::min(pX, mX), std::min(pZ, mZ)));
					occMap[x + NX * (y + NY * z)] = static_cast<uint8_t>(minCnt); // 0..4
				}
			}
		}

		static constexpr uint8_t AO_LUT[5] = {0, 64, 128, 192, 255};
		std::vector<uint8_t> occlusionMap(occMap.size());
		for (size_t i = 0; i < occMap.size(); i++)
		{
			occlusionMap[i] = AO_LUT[occMap[i]]; // 0, 64, 128, 192, 255
		}

		subMesh->m_OcclusionMap = std::move(occlusionMap);
	}

	void MeshBuilder::BuildFace(TextureAtlas& textureAtlas,
								SubChunkMesh& mesh,
								const BlockTextures& blockTextures,
								const FaceBuildDesc& f)
	{
		// ------ NORMAL PASS ---------
		for (size_t i = 0; i < blockTextures.faces.size(); i++)
		{
			if (blockTextures.faces[i].face == f.face)
			{
				const FaceTexture& faceTex = blockTextures.faces[i];
				auto uv = textureAtlas.GetAtlasEntry(faceTex.texture);
				AddFace(mesh, f, faceTex, uv);
			}
		}

		// ------ OVERLAY PASS ---------
		for (size_t i = 0; i < blockTextures.overlay.size(); i++)
		{
			if (blockTextures.overlay[i].face == f.face)
			{
				const FaceTexture& faceTex = blockTextures.overlay[i];
				auto uv = textureAtlas.GetAtlasEntry(faceTex.texture);
				AddFace(mesh, f, faceTex, uv);
			}
		}
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

	void MeshBuilder::Initialize()
	{
		m_BlockRegistry.Initialize();
	}

	void MeshBuilder::ReloadTextures()
	{
		m_BlockRegistry.ReloadTextures();
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

	const std::unordered_set<std::string>& MeshBuilder::GetAllRegisteredTextureNames() const
	{
		return m_BlockRegistry.GetAllTextureNames();
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
							  const FaceBuildDesc& f,
							  const FaceTexture& faceTexture,
							  const TextureAtlas::AtlasEntry& uv)
	{
		std::vector<SubChunkMesh::Vertex>* vertices = nullptr;
		std::vector<uint32_t>* indices = nullptr;

		switch (faceTexture.textureType)
		{
			case Transparency::Opaque:
				vertices = &mesh.m_VerticesOpaque;
				indices = &mesh.m_IndicesOpaque;
				break;

			case Transparency::Cutout:
				vertices = &mesh.m_VerticesCutout;
				indices = &mesh.m_IndicesCutout;
				break;

			case Transparency::Transparent:
				vertices = &mesh.m_VerticesTransparent;
				indices = &mesh.m_IndicesTransparent;
				break;
		}

		// ------ COMPUTE UV SUB-REGION ------

		// Determine the [s0,t0,s1,t1] sub-region within the atlas tile (0..1 range).
		// Priority: explicit per-face UV override wins; fall back to from/to-derived UVs.

		constexpr std::array<uint8_t, 4> kDefaultUV{0, 0, 16, 16};
		float s0, t0, s1, t1;

		if (faceTexture.uv != kDefaultUV)
		{
			// Explicit UV override from the block model JSON
			s0 = faceTexture.uv[0] / 16.0f;
			t0 = faceTexture.uv[1] / 16.0f;
			s1 = faceTexture.uv[2] / 16.0f;
			t1 = faceTexture.uv[3] / 16.0f;
		}
		else
		{
			// Derive UV sub-region from from/to geometry, following Minecraft's per-face UV convention.
			// MC UV coords: u maps to the horizontal axis on the face, v maps to the vertical (v=0 is top of image).
			const auto& fr = faceTexture.from;
			const auto& to = faceTexture.to;

			switch (f.face)
			{
				case Face::Top: // up:    u=X, v=Z
					s0 = fr.x / 16.0f;
					t0 = fr.z / 16.0f;
					s1 = to.x / 16.0f;
					t1 = to.z / 16.0f;
					break;
				case Face::Bottom: // down:  u=X, v=Z (Z flipped)
					s0 = fr.x / 16.0f;
					t0 = to.z / 16.0f;
					s1 = to.x / 16.0f;
					t1 = fr.z / 16.0f;
					break;
				case Face::Front: // north: u=X (right-to-left), v=Y (top-to-bottom = 16-Y)
					s0 = to.x / 16.0f;
					t0 = (16 - to.y) / 16.0f;
					s1 = fr.x / 16.0f;
					t1 = (16 - fr.y) / 16.0f;
					break;
				case Face::Back: // south: u=X (left-to-right), v=Y
					s0 = fr.x / 16.0f;
					t0 = (16 - to.y) / 16.0f;
					s1 = to.x / 16.0f;
					t1 = (16 - fr.y) / 16.0f;
					break;
				case Face::Left: // west:  u=Z (left-to-right), v=Y
					s0 = fr.z / 16.0f;
					t0 = (16 - to.y) / 16.0f;
					s1 = to.z / 16.0f;
					t1 = (16 - fr.y) / 16.0f;
					break;
				case Face::Right: // east:  u=Z (right-to-left), v=Y
					s0 = to.z / 16.0f;
					t0 = (16 - to.y) / 16.0f;
					s1 = fr.z / 16.0f;
					t1 = (16 - fr.y) / 16.0f;
					break;
				default:
					s0 = 0.0f;
					t0 = 0.0f;
					s1 = 1.0f;
					t1 = 1.0f;
					break;
			}
		}

		// Remap [s0,t0,s1,t1] into the atlas tile's UV space
		glm::vec2 tileSize = uv.uvMax - uv.uvMin;
		glm::vec2 uv0 = uv.uvMin + tileSize * glm::vec2(s0, t0);
		glm::vec2 uv1 = uv.uvMin + tileSize * glm::vec2(s1, t0);
		glm::vec2 uv2 = uv.uvMin + tileSize * glm::vec2(s1, t1);
		glm::vec2 uv3 = uv.uvMin + tileSize * glm::vec2(s0, t1);

		// ------ TINT HANDLING ------
		glm::ivec3 tint(255);

		switch (faceTexture.tintType)
		{
			case Tint::Grass:
				tint = glm::ivec3(95, 190, 60);
				break;

			case Tint::Foliage:
				tint = glm::ivec3(60, 170, 50);
				break;

			case Tint::Water:
				tint = glm::ivec3(77, 128, 255);
				break;

			default:
				break;
		}

		// ------ VERTEX CREATION ------
		uint32_t startIndex = static_cast<uint32_t>(vertices->size());

		auto makeVertex = [&](const glm::ivec3& p, const glm::vec2& uv, uint8_t occlusion)
		{
			SubChunkMesh::Vertex vert;

			vert.x = static_cast<uint16_t>(p.x);
			vert.y = static_cast<uint16_t>(p.y);
			vert.z = static_cast<uint16_t>(p.z);

			vert.texX = uv.x;
			vert.texY = uv.y;

			vert.facing = static_cast<uint8_t>(f.face);
			vert.occlusion = occlusion;

			vert.tintR = static_cast<uint8_t>(tint.r);
			vert.tintG = static_cast<uint8_t>(tint.g);
			vert.tintB = static_cast<uint8_t>(tint.b);

			return vert;
		};

		// ------ Add vertices -----
		vertices->push_back(makeVertex(*f.v[0], uv0, *f.o[0]));
		vertices->push_back(makeVertex(*f.v[1], uv1, *f.o[1]));
		vertices->push_back(makeVertex(*f.v[2], uv2, *f.o[2]));
		vertices->push_back(makeVertex(*f.v[3], uv3, *f.o[3]));

		// ------ Add indices -----
		if (!f.reverseWinding)
		{
			indices->push_back(startIndex + 0);
			indices->push_back(startIndex + 1);
			indices->push_back(startIndex + 2);

			indices->push_back(startIndex + 2);
			indices->push_back(startIndex + 3);
			indices->push_back(startIndex + 0);
		}
		else
		{
			indices->push_back(startIndex + 0);
			indices->push_back(startIndex + 3);
			indices->push_back(startIndex + 2);

			indices->push_back(startIndex + 2);
			indices->push_back(startIndex + 1);
			indices->push_back(startIndex + 0);
		}
	}

	MeshBuilder::PointsAndOcclusion MeshBuilder::GetPointsAndOcclusion(
		const BlockTextures& blockTextures, SubChunkMesh* mesh, const int lx, const int wy, const int lz)
	{
		switch (blockTextures.textureModel)
		{
			case Model::Block:
				return GetPointsAndOcclusionForBlock(mesh, lx, wy, lz);
			case Model::Cross:
				return GetPointsAndOcclusionForCross(mesh, lx, wy, lz);
			default:
				assert(false && "Unknown TextureModel");
				return PointsAndOcclusion();
		}
	}

	MeshBuilder::PointsAndOcclusion MeshBuilder::GetPointsAndOcclusionForBlock(
		SubChunkMesh* mesh, const int lx, const int wy, const int lz, const glm::u8vec3& from, const glm::u8vec3& to)
	{
		PointsAndOcclusion result;

		constexpr uint16_t subBlockSize = 16; // Assuming a sub-block is 16 units in size

		uint16_t ofnx = from.x;
		uint16_t ofpx = to.x;

		uint16_t ofny = from.y;
		uint16_t ofpy = to.y;

		uint16_t ofnz = from.z;
		uint16_t ofpz = to.z;

		result.p000 = glm::vec3(lx * subBlockSize + ofnx, wy * subBlockSize + ofny, lz * subBlockSize + ofnz);
		result.p001 = glm::vec3(lx * subBlockSize + ofnx, wy * subBlockSize + ofny, lz * subBlockSize + ofpz);
		result.p010 = glm::vec3(lx * subBlockSize + ofnx, wy * subBlockSize + ofpy, lz * subBlockSize + ofnz);
		result.p011 = glm::vec3(lx * subBlockSize + ofnx, wy * subBlockSize + ofpy, lz * subBlockSize + ofpz);

		result.p100 = glm::vec3(lx * subBlockSize + ofpx, wy * subBlockSize + ofny, lz * subBlockSize + ofnz);
		result.p101 = glm::vec3(lx * subBlockSize + ofpx, wy * subBlockSize + ofny, lz * subBlockSize + ofpz);
		result.p110 = glm::vec3(lx * subBlockSize + ofpx, wy * subBlockSize + ofpy, lz * subBlockSize + ofnz);
		result.p111 = glm::vec3(lx * subBlockSize + ofpx, wy * subBlockSize + ofpy, lz * subBlockSize + ofpz);

		constexpr int SIZE = WorldConstants::CHUNK_SIZE;
		const int ly = wy % SIZE;

		const int NX = SIZE + 1;
		const int NY = SIZE + 1;
		auto AO = [&](int dx, int dy, int dz) -> uint8_t
		{
			int ax = lx + dx;
			int ay = ly + dy;
			int az = lz + dz;
			return mesh->m_OcclusionMap[ax + NX * (ay + NY * az)];
		};

		result.o000 = AO(0, 0, 0);
		result.o001 = AO(0, 0, 1);
		result.o010 = AO(0, 1, 0);
		result.o011 = AO(0, 1, 1);

		result.o100 = AO(1, 0, 0);
		result.o101 = AO(1, 0, 1);
		result.o110 = AO(1, 1, 0);
		result.o111 = AO(1, 1, 1);

		return result;
	}

	MeshBuilder::PointsAndOcclusion
	MeshBuilder::GetPointsAndOcclusionForCross(SubChunkMesh* mesh, const int lx, const int wy, const int lz)
	{
		(void) mesh; // Cross model does not use occlusion values

		constexpr uint16_t subBlockSize = 16; // Assuming a sub-block is 16 units in size

		uint16_t ofnx = 0 * 16;
		uint16_t ofpx = 1 * 16;

		uint16_t ofny = 0 * 16;
		uint16_t ofpy = 1 * 16;

		uint16_t ofnz = 0 * 16;
		uint16_t ofpz = 1 * 16;

		PointsAndOcclusion result;
		result.p000 = glm::vec3(lx * subBlockSize + ofnx, wy * subBlockSize + ofny, lz * subBlockSize + ofnz);
		result.p001 = glm::vec3(lx * subBlockSize + ofnx, wy * subBlockSize + ofny, lz * subBlockSize + ofpz);
		result.p010 = glm::vec3(lx * subBlockSize + ofnx, wy * subBlockSize + ofpy, lz * subBlockSize + ofnz);
		result.p011 = glm::vec3(lx * subBlockSize + ofnx, wy * subBlockSize + ofpy, lz * subBlockSize + ofpz);
		result.p100 = glm::vec3(lx * subBlockSize + ofpx, wy * subBlockSize + ofny, lz * subBlockSize + ofnz);
		result.p101 = glm::vec3(lx * subBlockSize + ofpx, wy * subBlockSize + ofny, lz * subBlockSize + ofpz);
		result.p110 = glm::vec3(lx * subBlockSize + ofpx, wy * subBlockSize + ofpy, lz * subBlockSize + ofnz);
		result.p111 = glm::vec3(lx * subBlockSize + ofpx, wy * subBlockSize + ofpy, lz * subBlockSize + ofpz);
		result.o000 = 0;
		result.o001 = 0;
		result.o010 = 0;
		result.o011 = 0;
		result.o100 = 0;
		result.o101 = 0;
		result.o110 = 0;
		result.o111 = 0;
		return result;
	}

	std::vector<MeshBuilder::FaceBuildDesc> MeshBuilder::GetFaceBuildDescs(const BlockTextures& blockTextures,
																		   const PointsAndOcclusion& pao)
	{
		std::vector<MeshBuilder::FaceBuildDesc> desc;

		switch (blockTextures.textureModel)
		{
			case Model::Block:
				desc = GetBlockFaceBuildDescs(pao);
				break;
			case Model::Cross:
				desc = GetCrossFaceBuildDescs(pao);
				break;
			default:
				assert(false && "Unknown TextureModel");
				break;
		}

		return desc;
	}

	std::vector<MeshBuilder::FaceBuildDesc> MeshBuilder::GetBlockFaceBuildDescs(const PointsAndOcclusion& pao)
	{
		return {
			{Face::Top, {&pao.p011, &pao.p111, &pao.p110, &pao.p010}, {&pao.o011, &pao.o111, &pao.o110, &pao.o010}},
			{Face::Bottom, {&pao.p000, &pao.p100, &pao.p101, &pao.p001}, {&pao.o000, &pao.o100, &pao.o101, &pao.o001}},
			{Face::Front, {&pao.p001, &pao.p101, &pao.p111, &pao.p011}, {&pao.o001, &pao.o101, &pao.o111, &pao.o011}},
			{Face::Back, {&pao.p100, &pao.p000, &pao.p010, &pao.p110}, {&pao.o100, &pao.o000, &pao.o010, &pao.o110}},
			{Face::Right, {&pao.p101, &pao.p100, &pao.p110, &pao.p111}, {&pao.o101, &pao.o100, &pao.o110, &pao.o111}},
			{Face::Left, {&pao.p000, &pao.p001, &pao.p011, &pao.p010}, {&pao.o000, &pao.o001, &pao.o011, &pao.o010}},
		};
	}

	std::vector<MeshBuilder::FaceBuildDesc> MeshBuilder::GetCrossFaceBuildDescs(const PointsAndOcclusion& pao)
	{
		// The cross model consists of two quads that intersect each other. Each quad has a front and back face (For correct FACE CULLING)
		return {
			// Plane 1
			FaceBuildDesc{
				Face::Top, {&pao.p000, &pao.p101, &pao.p111, &pao.p010}, {&pao.o000, &pao.o101, &pao.o111, &pao.o010}},

			// Plane 2
			FaceBuildDesc{
				Face::Top, {&pao.p100, &pao.p001, &pao.p011, &pao.p110}, {&pao.o100, &pao.o001, &pao.o011, &pao.o110}},

			// Plane 1
			FaceBuildDesc{Face::Top,
						  {&pao.p000, &pao.p101, &pao.p111, &pao.p010},
						  {&pao.o000, &pao.o101, &pao.o111, &pao.o010},
						  true},

			// Plane 2
			FaceBuildDesc{Face::Top,
						  {&pao.p100, &pao.p001, &pao.p011, &pao.p110},
						  {&pao.o100, &pao.o001, &pao.o011, &pao.o110},
						  true},

		};
	}

} // namespace onion::voxel
