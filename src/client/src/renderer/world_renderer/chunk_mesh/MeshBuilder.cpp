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

	static inline glm::ivec3 OrientationToVector(BlockState::Orientation o)
	{
		switch (o)
		{
			case BlockState::Orientation::North:
				return {0, 0, 1};
			case BlockState::Orientation::South:
				return {0, 0, -1};
			case BlockState::Orientation::East:
				return {1, 0, 0};
			case BlockState::Orientation::West:
				return {-1, 0, 0};
			case BlockState::Orientation::Up:
				return {0, 1, 0};
			case BlockState::Orientation::Down:
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

	static inline BlockFace GetTextureFaceForWorldFace(BlockFace worldFace, const BlockState& block)
	{
		if (block.Facing == BlockState::Orientation::None || block.Top == BlockState::Orientation::None)
		{
			return worldFace;
		}

		glm::ivec3 forward = OrientationToVector(block.Facing);
		glm::ivec3 up = OrientationToVector(block.Top);
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

	static int GetRotationSteps(const BlockState& block, BlockFace worldFace)
	{
		if (block.Facing == BlockState::Orientation::None || block.Facing == BlockState::Orientation::Up ||
			block.Facing == BlockState::Orientation::Down)
		{
			return 0;
		}

		if (block.Facing == BlockState::Orientation::North || block.Facing == BlockState::Orientation::South)
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

	std::array<bool, 6> GetFaceVisibility(const BlockState& block, const std::array<BlockState, 6>& neighbors)
	{
		std::array<bool, 6> visibility{};

		for (int i = 0; i < 6; i++)
		{
			// Water is only visible when next to air
			if (block.ID == BlockId::Water && neighbors[i].ID != BlockId::Air)
			{
				visibility[i] = false;
				continue;
			}

			if (block.ID == BlockId::OakLeaves && neighbors[i].ID == BlockId::OakLeaves)
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
						float wx = chunkPos.x * SIZE + x;
						float wy = SIZE * sub + y;
						float wz = chunkPos.y * SIZE + z;

						// ------ Get Neighboring Blocks ------
						std::array<BlockState, 6> neighbors;

						// Top (+y)
						if (localPos.y + 1 < subChunkCount * SIZE)
							neighbors[(int) BlockFace::Top] = chunk->GetBlock(glm::ivec3(x, localPos.y + 1, z));
						else
							neighbors[(int) BlockFace::Top] = BlockState(BlockId::Air); // Air block if above the world

						// Bottom (-y)
						if (localPos.y - 1 >= 0)
							neighbors[(int) BlockFace::Bottom] = chunk->GetBlock(glm::ivec3(x, localPos.y - 1, z));
						else
							neighbors[(int) BlockFace::Bottom] =
								BlockState(BlockId::Air); // Air block if below the world

						// Front (+z)
						if (z + 1 < SIZE)
							neighbors[(int) BlockFace::Front] = chunk->GetBlock(glm::ivec3(x, localPos.y, z + 1));
						else
						{
							neighbors[(int) BlockFace::Front] = adjacentPosZ
								? adjacentPosZ->GetBlock(glm::ivec3(x, localPos.y, 0))
								: BlockState(BlockId::Stone);
						}

						// Back (-z)
						if (z - 1 >= 0)
							neighbors[(int) BlockFace::Back] = chunk->GetBlock(glm::ivec3(x, localPos.y, z - 1));
						else
						{
							neighbors[(int) BlockFace::Back] = adjacentNegZ
								? adjacentNegZ->GetBlock(glm::ivec3(x, localPos.y, SIZE - 1))
								: BlockState(BlockId::Stone);
						}

						// Right (+x)
						if (x + 1 < SIZE)
							neighbors[(int) BlockFace::Right] = chunk->GetBlock(glm::ivec3(x + 1, localPos.y, z));
						else
						{
							neighbors[(int) BlockFace::Right] = adjacentPosX
								? adjacentPosX->GetBlock(glm::ivec3(0, localPos.y, z))
								: BlockState(BlockId::Stone);
						}

						// Left (-x)
						if (x - 1 >= 0)
							neighbors[(int) BlockFace::Left] = chunk->GetBlock(glm::ivec3(x - 1, localPos.y, z));
						else
						{
							neighbors[(int) BlockFace::Left] = adjacentNegX
								? adjacentNegX->GetBlock(glm::ivec3(SIZE - 1, localPos.y, z))
								: BlockState(BlockId::Stone);
						}

						// ------ Determine Face Visibility ------
						const std::array<bool, 6> faceVisible = GetFaceVisibility(block, neighbors);

						// If no faces are visible, skip this block
						if (!std::any_of(faceVisible.begin(), faceVisible.end(), [](bool v) { return v; }))
							continue;

						// ------ Get Block Textures ------
						const BlockTextures& blockTextures = m_BlockRegistry.Get(block.ID);

						// ------ Build Mesh ------
						glm::ivec3 p000(x, wy, z);
						glm::ivec3 p001(x, wy, z + 1);
						glm::ivec3 p010(x, wy + 1, z);
						glm::ivec3 p011(x, wy + 1, z + 1);

						glm::ivec3 p100(x + 1, wy, z);
						glm::ivec3 p101(x + 1, wy, z + 1);
						glm::ivec3 p110(x + 1, wy + 1, z);
						glm::ivec3 p111(x + 1, wy + 1, z + 1);

						const int NX = SIZE + 1;
						const int NY = SIZE + 1;
						auto AO = [&](int dx, int dy, int dz) -> uint8_t
						{
							int ax = x + dx;
							int ay = y + dy;
							int az = z + dz;
							return mesh->m_OcclusionMap[ax + NX * (ay + NY * az)];
						};

						uint8_t o000 = AO(0, 0, 0);
						uint8_t o001 = AO(0, 0, 1);
						uint8_t o010 = AO(0, 1, 0);
						uint8_t o011 = AO(0, 1, 1);

						uint8_t o100 = AO(1, 0, 0);
						uint8_t o101 = AO(1, 0, 1);
						uint8_t o110 = AO(1, 1, 0);
						uint8_t o111 = AO(1, 1, 1);

						auto buildFace = [&](BlockFace worldFace,
											 BlockFace textureFace,
											 const glm::ivec3& v0,
											 const glm::ivec3& v1,
											 const glm::ivec3& v2,
											 const glm::ivec3& v3,
											 const uint8_t o0,
											 const uint8_t o1,
											 const uint8_t o2,
											 const uint8_t o3)
						{
							const FaceTexture& faceTex = blockTextures.faces[(size_t) textureFace];

							auto uv = m_TextureAtlas->GetAtlasEntry(faceTex.texture);

							AddFace(*mesh,
									v0,
									v1,
									v2,
									v3,
									o0,
									o1,
									o2,
									o3,
									worldFace,
									block,
									faceTex,
									uv,
									blockTextures.rotationType);

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
										o0,
										o1,
										o2,
										o3,
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
									  p010,
									  o011,
									  o111,
									  o110,
									  o010);
						}

						if (faceVisible[(int) BlockFace::Bottom])
						{
							buildFace(BlockFace::Bottom,
									  GetTextureFaceForWorldFace(BlockFace::Bottom, block),
									  p000,
									  p100,
									  p101,
									  p001,
									  o000,
									  o100,
									  o101,
									  o001);
						}

						if (faceVisible[(int) BlockFace::Front])
						{
							buildFace(BlockFace::Front,
									  GetTextureFaceForWorldFace(BlockFace::Front, block),
									  p001,
									  p101,
									  p111,
									  p011,
									  o001,
									  o101,
									  o111,
									  o011);
						}

						if (faceVisible[(int) BlockFace::Back])
						{
							buildFace(BlockFace::Back,
									  GetTextureFaceForWorldFace(BlockFace::Back, block),
									  p100,
									  p000,
									  p010,
									  p110,
									  o100,
									  o000,
									  o010,
									  o110);
						}

						if (faceVisible[(int) BlockFace::Right])
						{
							buildFace(BlockFace::Right,
									  GetTextureFaceForWorldFace(BlockFace::Right, block),
									  p101,
									  p100,
									  p110,
									  p111,
									  o101,
									  o100,
									  o110,
									  o111);
						}

						if (faceVisible[(int) BlockFace::Left])
						{
							buildFace(BlockFace::Left,
									  GetTextureFaceForWorldFace(BlockFace::Left, block),
									  p000,
									  p001,
									  p011,
									  p010,
									  o000,
									  o001,
									  o011,
									  o010);
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
				constexpr Row FULL_X = Row((1ull << SX) - 1ull);
				constexpr Row FULL_Z = Row((1ull << SZ) - 1ull);
				constexpr Row FULL_Y = Row((1ull << SY) - 1ull);

				for (int y = 0; y < SY; ++y)
				{
					for (int z = 0; z < SZ; ++z)
						solidX[y][z] = FULL_X;
					for (int x = 0; x < SX; ++x)
						solidZ[y][x] = FULL_Z;
				}
				for (int z = 0; z < SZ; ++z)
					for (int x = 0; x < SX; ++x)
						solidY[z][x] = FULL_Y;
			}
		}
		else
		{
			for (int y = 0; y < SY; ++y)
			{
				for (int z = 0; z < SZ; ++z)
				{
					Row rowX = 0;
					for (int x = 0; x < SX; ++x)
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
		const int baseX = chunkPos.x * SX;
		const int baseZ = chunkPos.y * SZ;

		// X- (x = -1) and X+ (x = SX)
		for (int ly = 0; ly < SY; ly++)
		{
			for (int lz = 0; lz < SZ; lz++)
			{

				// X-
				{
					BlockId blockId =
						adjacentNegX ? adjacentNegX->GetBlock({SX - 1, ly + yMini, lz}).ID : BlockId::Stone;
					nbrXneg[ly][lz] = BlockState::IsOpaque(blockId) ? 1 : 0;
				}
				// X+
				{
					BlockId blockId = adjacentPosX ? adjacentPosX->GetBlock({0, ly + yMini, lz}).ID : BlockId::Stone;
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
					BlockId blockId =
						adjacentNegZ ? adjacentNegZ->GetBlock({lx, ly + yMini, SZ - 1}).ID : BlockId::Stone;
					nbrZneg[ly][lx] = BlockState::IsOpaque(blockId) ? 1 : 0;
				}
				// Z+
				{
					BlockId blockId = adjacentPosZ ? adjacentPosZ->GetBlock({lx, ly + yMini, 0}).ID : BlockId::Stone;
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

		for (int z = 0; z < NZ; ++z)
		{
			for (int y = 0; y < NY; ++y)
			{
				for (int x = 0; x < NX; ++x)
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
		for (size_t i = 0; i < occMap.size(); ++i)
		{
			occlusionMap[i] = AO_LUT[occMap[i]]; // 0, 64, 128, 192, 255
		}

		subMesh->m_OcclusionMap = std::move(occlusionMap);
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
							  const glm::ivec3& v0,
							  const glm::ivec3& v1,
							  const glm::ivec3& v2,
							  const glm::ivec3& v3,
							  const uint8_t o0,
							  const uint8_t o1,
							  const uint8_t o2,
							  const uint8_t o3,
							  BlockFace face,
							  const BlockState& block,
							  const FaceTexture& faceTexture,
							  const TextureAtlas::AtlasEntry& uv,
							  BlockState::RotationType rotationType)
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

		if (rotationType == BlockState::RotationType::Pillar)
		{
			int rotationSteps = GetRotationSteps(block, face);
			RotateUVs(uv0, uv1, uv2, uv3, rotationSteps);
		}

		// ------ TINT HANDLING ------
		glm::ivec3 tint(255);

		switch (faceTexture.tintType)
		{
			case TintType::Grass:
				tint = glm::ivec3(95, 190, 60);
				break;

			case TintType::Foliage:
				tint = glm::ivec3(60, 170, 50);
				break;

			case TintType::Water:
				tint = glm::ivec3(77, 128, 255);
				break;

			default:
				break;
		}

		// ------ VERTEX CREATION ------
		uint16_t startIndex = static_cast<uint16_t>(vertices->size());

		auto makeVertex = [&](const glm::ivec3& p, const glm::vec2& uv, uint8_t occlusion)
		{
			SubChunkMesh::Vertex vert;

			vert.x = static_cast<uint8_t>(p.x);
			vert.y = static_cast<uint8_t>(p.y);
			vert.z = static_cast<uint8_t>(p.z);

			vert.texX = uv.x;
			vert.texY = uv.y;

			vert.facing = static_cast<uint8_t>(face);
			vert.occlusion = occlusion;

			vert.tintR = static_cast<uint8_t>(tint.r);
			vert.tintG = static_cast<uint8_t>(tint.g);
			vert.tintB = static_cast<uint8_t>(tint.b);

			return vert;
		};

		// ------ Add vertices -----
		vertices->push_back(makeVertex(v0, uv0, o0));
		vertices->push_back(makeVertex(v1, uv1, o1));
		vertices->push_back(makeVertex(v2, uv2, o2));
		vertices->push_back(makeVertex(v3, uv3, o3));

		// ------ Add indices -----
		indices->push_back(startIndex + 0);
		indices->push_back(startIndex + 1);
		indices->push_back(startIndex + 2);

		indices->push_back(startIndex + 2);
		indices->push_back(startIndex + 3);
		indices->push_back(startIndex + 0);
	}
} // namespace onion::voxel
