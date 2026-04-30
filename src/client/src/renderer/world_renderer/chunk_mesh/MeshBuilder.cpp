#include "MeshBuilder.hpp"

#include <numeric>

#include <glm/gtc/matrix_transform.hpp>
#include <shared/utils/Stopwatch.hpp>

namespace onion::voxel
{
	MeshBuilder::MeshBuilder(std::shared_ptr<WorldManager> worldManager, std::shared_ptr<TextureAtlas> textureAtlas)
		: m_WorldManager(worldManager), m_BlockRenderRegistry(textureAtlas), m_TextureAtlas(textureAtlas)
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
						const BlockTextures& blockTextures = m_BlockRenderRegistry.Get(block.ID, block.VariantIndex);

						// ------ Build Mesh ------
						// Build each face individually using its own element geometry (from/to)
						for (size_t i = 0; i < blockTextures.faces.size(); i++)
						{
							const int& faceIdx = (int) blockTextures.faces[i].face;

							if (!faceVisible[faceIdx])
								continue;

							const TextureInfo& faceTexture = blockTextures.faces[i];

							if (faceTexture.texture == UINT16_MAX)
							{
								continue;
							}

							PointsAndOcclusion pao = GetPointsAndOcclusion(mesh.get(), x, wy, z, faceTexture);

							std::vector<FaceBuildDesc> faceDescs = GetBlockFaceBuildDescs(pao);
							// Only emit the descriptor matching this face index, and only
							// draw the specific face entry [i] that owns this pao — not all
							// entries sharing the same face direction (which would mix UVs
							// from different elements onto the wrong geometry).
							for (const auto& f : faceDescs)
							{
								if ((int) f.face != faceIdx)
									continue;

								// Normal pass: this entry only
								auto atlasEntry = m_TextureAtlas->GetAtlasEntry(faceTexture.texture);
								AddFace(*mesh, f, faceTexture, atlasEntry);

								// Overlay pass: all overlay entries for this face direction
								for (const auto& overlayTex : blockTextures.overlay)
								{
									if (overlayTex.face != faceTexture.face)
										continue;
									auto overlayAtlas = m_TextureAtlas->GetAtlasEntry(overlayTex.texture);
									AddFace(*mesh, f, overlayTex, overlayAtlas);
								}
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
			const bool countsInAO = BlockstateRegistry::CountsInAO(mono.ID, mono.VariantIndex);
			if (countsInAO)
			{
				const Row FULL_X = ~Row(0);
				const Row FULL_Z = ~Row(0);
				const Row FULL_Y = ~Row(0);

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
						const BlockState blockstate =
							chunk->GetBlock({static_cast<int>(x), static_cast<int>(y) + yMini, static_cast<int>(z)});
						const bool countsInAO = BlockstateRegistry::CountsInAO(blockstate.ID, blockstate.VariantIndex);
						rowX |= Row(countsInAO) << x;		  // along X in [y][z]
						solidZ[y][x] |= Row(countsInAO) << z; // along Z in [y][x]
						solidY[z][x] |= Row(countsInAO) << y; // along Y in [z][x]
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
					BlockState block =
						adjacentNegX ? adjacentNegX->GetBlock({SX - 1, ly + yMini, lz}) : BlockState(BlockId::Air);
					nbrXneg[ly][lz] = BlockstateRegistry::CountsInAO(block.ID, block.VariantIndex) ? 1 : 0;
				}
				// X+
				{
					BlockState block =
						adjacentPosX ? adjacentPosX->GetBlock({0, ly + yMini, lz}) : BlockState(BlockId::Air);
					nbrXpos[ly][lz] = BlockstateRegistry::CountsInAO(block.ID, block.VariantIndex) ? 1 : 0;
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
					BlockState block =
						adjacentNegZ ? adjacentNegZ->GetBlock({lx, ly + yMini, SZ - 1}) : BlockState(BlockId::Air);
					nbrZneg[ly][lx] = BlockstateRegistry::CountsInAO(block.ID, block.VariantIndex) ? 1 : 0;
				}
				// Z+
				{
					BlockState block =
						adjacentPosZ ? adjacentPosZ->GetBlock({lx, ly + yMini, 0}) : BlockState(BlockId::Air);
					nbrZpos[ly][lx] = BlockstateRegistry::CountsInAO(block.ID, block.VariantIndex) ? 1 : 0;
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
					BlockState block = chunk->GetBlock({lx, yMini - 1, lz});
					nbrYneg[lz][lx] = BlockstateRegistry::CountsInAO(block.ID, block.VariantIndex) ? 1 : 0;
				}
				// Y+
				{
					BlockState block = chunk->GetBlock({lx, yMini + SY, lz});
					nbrYpos[lz][lx] = BlockstateRegistry::CountsInAO(block.ID, block.VariantIndex) ? 1 : 0;
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
				const TextureInfo& faceTex = blockTextures.faces[i];
				auto uv = textureAtlas.GetAtlasEntry(faceTex.texture);
				AddFace(mesh, f, faceTex, uv);
			}
		}

		// ------ OVERLAY PASS ---------
		for (size_t i = 0; i < blockTextures.overlay.size(); i++)
		{
			if (blockTextures.overlay[i].face == f.face)
			{
				const TextureInfo& faceTex = blockTextures.overlay[i];
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
		m_BlockRenderRegistry.Initialize();
	}

	void MeshBuilder::ReloadTextures()
	{
		m_BlockRenderRegistry.ReloadTextures();
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

	void MeshBuilder::UpdateUiBlockMesh(const std::shared_ptr<UiBlockMesh> uiBlockMesh) const
	{
		std::unique_lock lock(uiBlockMesh->m_Mutex);

		// Store the atlas so Render() can bind it
		uiBlockMesh->m_TextureAtlas = m_TextureAtlas;

		// Clear existing geometry
		uiBlockMesh->m_VerticesOpaque.clear();
		uiBlockMesh->m_IndicesOpaque.clear();
		uiBlockMesh->m_VerticesCutout.clear();
		uiBlockMesh->m_IndicesCutout.clear();
		uiBlockMesh->m_VerticesTransparent.clear();
		uiBlockMesh->m_IndicesTransparent.clear();

		const Inventory& inventory = uiBlockMesh->m_Inventory;
		const glm::vec2& slotSize = uiBlockMesh->m_SlotSize;
		const glm::vec2& slotPadding = uiBlockMesh->m_SlotPadding;

		const auto& blockstateRegistry = BlockstateRegistry::Get();

		const int rows = inventory.Rows();
		const int cols = inventory.Columns();

		for (int row = 0; row < rows; ++row)
		{
			for (int col = 0; col < cols; ++col)
			{
				const BlockId blockId = inventory.At(row, col);
				if (blockId == BlockId::Air)
					continue;

				// Slot top-left in normalized screen space
				const float slotX = col * (slotSize.x + slotPadding.x);
				const float slotY = row * (slotSize.y + slotPadding.y);

				// Fetch Gui display transform from block model
				auto registryIt = blockstateRegistry.find(blockId);
				if (registryIt == blockstateRegistry.end() || registryIt->second.empty())
					continue;

				const auto& variants = registryIt->second;
				const uint8_t guiVariantIdx = BlockstateRegistry::GetVariantIndex(blockId, {{"gui", "true"}});
				const size_t bestVariantIdx =
					static_cast<size_t>(guiVariantIdx) < variants.size() ? static_cast<size_t>(guiVariantIdx) : 0;

				const BlockModel::DisplayInfo& gui = variants[bestVariantIdx].Model.ModelDisplay.Gui;

				// Build transform matrix.
				// The unit cube spans [-0.5 .. 0.5] on each axis.
				// We apply: scale(slot) → scale(gui) → Rx → Ry → Rz → translate(gui) → translate(slot center)
				// gui.Translation is in MC model units where 1 block = 16 units.
				// gui.Scale is a direct multiplier on the block.
				const glm::vec3 slotCenter(slotX + slotSize.x * 0.5f, slotY + slotSize.y * 0.5f, 0.0f);
				const float border = uiBlockMesh->GetSlotBorder();
				const float blockScreenSize = slotSize.x - 2.f * border; // shrink block by inner border

				glm::mat4 transform = glm::mat4(1.0f);
				transform = glm::translate(transform, slotCenter);
				transform = glm::translate(transform, gui.Translation / 16.0f * blockScreenSize);
				transform = glm::rotate(transform, glm::radians(gui.Rotation.z), glm::vec3(0, 0, 1));
				transform = glm::rotate(transform, glm::radians(-gui.Rotation.x), glm::vec3(1, 0, 0));
				transform = glm::rotate(transform, glm::radians(gui.Rotation.y), glm::vec3(0, 1, 0));
				transform = glm::scale(transform, gui.Scale * (-blockScreenSize));

				// Compute the 8 corners of the unit cube through the transform.
				// Convention matches GetPointsAndOcclusion: pXYZ where X=+x,Y=+y,Z=+z (1=max, 0=min)
				auto tfm = [&](float x, float y, float z) -> glm::vec3
				{ return glm::vec3(transform * glm::vec4(x, y, z, 1.0f)); };

				// Build face quads — one PAO per element since elements may differ in size (e.g. slabs)
				const BlockTextures& blockTextures =
					m_BlockRenderRegistry.Get(blockId, static_cast<uint8_t>(bestVariantIdx));

				for (const FaceBuildDesc& f : GetBlockFaceBuildDescs(
						 // Dummy full-cube PAO just to enumerate all 6 face directions;
						 // actual per-element PAO is computed below per TextureInfo
						 [&]()
						 {
							 PointsAndOcclusion cube;
							 cube.p000 = tfm(-0.5f, -0.5f, -0.5f);
							 cube.p001 = tfm(-0.5f, -0.5f, +0.5f);
							 cube.p010 = tfm(-0.5f, +0.5f, -0.5f);
							 cube.p011 = tfm(-0.5f, +0.5f, +0.5f);
							 cube.p100 = tfm(+0.5f, -0.5f, -0.5f);
							 cube.p101 = tfm(+0.5f, -0.5f, +0.5f);
							 cube.p110 = tfm(+0.5f, +0.5f, -0.5f);
							 cube.p111 = tfm(+0.5f, +0.5f, +0.5f);
							 return cube;
						 }()))
				{
					const int faceIdx = static_cast<int>(f.face);

					// Normal pass
					for (const TextureInfo& faceTex : blockTextures.faces)
					{
						if (static_cast<int>(faceTex.face) != faceIdx)
							continue;
						if (faceTex.texture == UINT16_MAX)
							continue;

						// Compute element-accurate PAO for this specific TextureInfo
						const PointsAndOcclusion elemPao = [&]()
						{
							PointsAndOcclusion p = GetElementLocalPao(faceTex);
							p.p000 = tfm(p.p000.x, p.p000.y, p.p000.z);
							p.p001 = tfm(p.p001.x, p.p001.y, p.p001.z);
							p.p010 = tfm(p.p010.x, p.p010.y, p.p010.z);
							p.p011 = tfm(p.p011.x, p.p011.y, p.p011.z);
							p.p100 = tfm(p.p100.x, p.p100.y, p.p100.z);
							p.p101 = tfm(p.p101.x, p.p101.y, p.p101.z);
							p.p110 = tfm(p.p110.x, p.p110.y, p.p110.z);
							p.p111 = tfm(p.p111.x, p.p111.y, p.p111.z);
							return p;
						}();

						const std::vector<FaceBuildDesc> elemDescs = GetBlockFaceBuildDescs(elemPao);
						for (const FaceBuildDesc& ef : elemDescs)
						{
							if (static_cast<int>(ef.face) != faceIdx)
								continue;
							const auto atlasEntry = m_TextureAtlas->GetAtlasEntry(faceTex.texture);
							AddUiFace(*uiBlockMesh, ef, faceTex, atlasEntry);
						}
					}

					// Overlay pass
					for (const TextureInfo& overlayTex : blockTextures.overlay)
					{
						if (static_cast<int>(overlayTex.face) != faceIdx)
							continue;
						if (overlayTex.texture == UINT16_MAX)
							continue;

						const PointsAndOcclusion elemPao = [&]()
						{
							PointsAndOcclusion p = GetElementLocalPao(overlayTex);
							p.p000 = tfm(p.p000.x, p.p000.y, p.p000.z);
							p.p001 = tfm(p.p001.x, p.p001.y, p.p001.z);
							p.p010 = tfm(p.p010.x, p.p010.y, p.p010.z);
							p.p011 = tfm(p.p011.x, p.p011.y, p.p011.z);
							p.p100 = tfm(p.p100.x, p.p100.y, p.p100.z);
							p.p101 = tfm(p.p101.x, p.p101.y, p.p101.z);
							p.p110 = tfm(p.p110.x, p.p110.y, p.p110.z);
							p.p111 = tfm(p.p111.x, p.p111.y, p.p111.z);
							return p;
						}();

						const std::vector<FaceBuildDesc> elemDescs = GetBlockFaceBuildDescs(elemPao);
						for (const FaceBuildDesc& ef : elemDescs)
						{
							if (static_cast<int>(ef.face) != faceIdx)
								continue;
							const auto overlayAtlas = m_TextureAtlas->GetAtlasEntry(overlayTex.texture);
							AddUiFace(*uiBlockMesh, ef, overlayTex, overlayAtlas);
						}
					}
				}
			}
		}

		uiBlockMesh->SetDirty(false);
		uiBlockMesh->BuffersUpdated();
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
		return m_BlockRenderRegistry.GetAllTextureNames();
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
							  const TextureInfo& faceTexture,
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

		float s0 = faceTexture.uv[0] / 16.0f;
		float s1 = faceTexture.uv[2] / 16.0f;
		float t0 = 1.0f - faceTexture.uv[3] / 16.0f; // MC v2, flipped to OpenGL
		float t1 = 1.0f - faceTexture.uv[1] / 16.0f; // MC v1, flipped to OpenGL

		// Remap [s0,t0,s1,t1] into the atlas tile's UV space
		glm::vec2 tileSize = uv.uvMax - uv.uvMin;
		glm::vec2 uv0 = uv.uvMin + tileSize * glm::vec2(s0, t0);
		glm::vec2 uv1 = uv.uvMin + tileSize * glm::vec2(s1, t0);
		glm::vec2 uv2 = uv.uvMin + tileSize * glm::vec2(s1, t1);
		glm::vec2 uv3 = uv.uvMin + tileSize * glm::vec2(s0, t1);

		// Apply per-face UV rotation (0, 90, 180, 270 degrees CW).
		// A cyclic permutation of the four corners rotates the texture on the quad.
		const int uvSteps = ((faceTexture.uvRotation / 90) % 4 + 4) % 4;
		if (uvSteps != 0)
		{
			// Rotate the corner array by `uvSteps` positions (each step = 90° CW)
			std::array<glm::vec2, 4> corners{uv0, uv1, uv2, uv3};
			uv0 = corners[(0 + uvSteps) % 4];
			uv1 = corners[(1 + uvSteps) % 4];
			uv2 = corners[(2 + uvSteps) % 4];
			uv3 = corners[(3 + uvSteps) % 4];
		}

		// ------ TINT HANDLING ------
		glm::ivec3 tint(255);

		switch (faceTexture.tintType)
		{
			case Tint::Grass:
				tint = glm::ivec3(95, 190, 60);
				break;

			case Tint::Water:
				tint = glm::ivec3(77, 128, 255);
				break;

			default:
				break;
		}

		// ------ VERTEX CREATION ------
		uint32_t startIndex = static_cast<uint32_t>(vertices->size());

		auto makeVertex = [&](const glm::vec3& p, const glm::vec2& uv, uint8_t occlusion)
		{
			SubChunkMesh::Vertex vert;

			vert.x = static_cast<int16_t>(std::round(p.x));
			vert.y = static_cast<int16_t>(std::round(p.y));
			vert.z = static_cast<int16_t>(std::round(p.z));

			vert.texX = uv.x;
			vert.texY = uv.y;

			vert.facing = vert.facing = static_cast<uint8_t>(faceTexture.shade ? f.face : Face::Top);
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

	void MeshBuilder::AddUiFace(UiBlockMesh& mesh,
								const FaceBuildDesc& f,
								const TextureInfo& faceTexture,
								const TextureAtlas::AtlasEntry& uv)
	{
		std::vector<UiBlockMesh::Vertex>* vertices = nullptr;
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
		float s0 = faceTexture.uv[0] / 16.0f;
		float s1 = faceTexture.uv[2] / 16.0f;
		float t0 = 1.0f - faceTexture.uv[3] / 16.0f; // MC v2, flipped to OpenGL
		float t1 = 1.0f - faceTexture.uv[1] / 16.0f; // MC v1, flipped to OpenGL

		glm::vec2 tileSize = uv.uvMax - uv.uvMin;
		glm::vec2 uv0 = uv.uvMin + tileSize * glm::vec2(s0, t0);
		glm::vec2 uv1 = uv.uvMin + tileSize * glm::vec2(s1, t0);
		glm::vec2 uv2 = uv.uvMin + tileSize * glm::vec2(s1, t1);
		glm::vec2 uv3 = uv.uvMin + tileSize * glm::vec2(s0, t1);

		// Apply per-face UV rotation (0, 90, 180, 270 degrees CW)
		const int uvSteps = ((faceTexture.uvRotation / 90) % 4 + 4) % 4;
		if (uvSteps != 0)
		{
			std::array<glm::vec2, 4> corners{uv0, uv1, uv2, uv3};
			uv0 = corners[(0 + uvSteps) % 4];
			uv1 = corners[(1 + uvSteps) % 4];
			uv2 = corners[(2 + uvSteps) % 4];
			uv3 = corners[(3 + uvSteps) % 4];
		}

		// ------ TINT HANDLING ------
		glm::ivec3 tint(255);

		switch (faceTexture.tintType)
		{
			case Tint::Grass:
				tint = glm::ivec3(95, 190, 60);
				break;

			case Tint::Water:
				tint = glm::ivec3(77, 128, 255);
				break;

			default:
				break;
		}

		// ------ VERTEX CREATION ------
		uint32_t startIndex = static_cast<uint32_t>(vertices->size());

		auto makeVertex = [&](const glm::vec3& p, const glm::vec2& texUv)
		{
			UiBlockMesh::Vertex vert;

			vert.x = p.x;
			vert.y = p.y;
			vert.z = p.z;

			vert.texX = texUv.x;
			vert.texY = texUv.y;

			vert.facing = static_cast<uint8_t>(faceTexture.shade ? f.face : Face::Top);

			vert.tintR = static_cast<uint8_t>(tint.r);
			vert.tintG = static_cast<uint8_t>(tint.g);
			vert.tintB = static_cast<uint8_t>(tint.b);

			return vert;
		};

		// ------ Add vertices -----
		vertices->push_back(makeVertex(*f.v[0], uv0));
		vertices->push_back(makeVertex(*f.v[1], uv1));
		vertices->push_back(makeVertex(*f.v[2], uv2));
		vertices->push_back(makeVertex(*f.v[3], uv3));

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

	MeshBuilder::PointsAndOcclusion MeshBuilder::GetElementLocalPao(const TextureInfo& textureInfo)
	{
		// Map MC units (0-16) to local block space (-0.5 .. +0.5)
		float nx = textureInfo.from.x / 16.0f - 0.5f;
		float px = textureInfo.to.x / 16.0f - 0.5f;
		float ny = textureInfo.from.y / 16.0f - 0.5f;
		float py = textureInfo.to.y / 16.0f - 0.5f;
		float nz = textureInfo.from.z / 16.0f - 0.5f;
		float pz = textureInfo.to.z / 16.0f - 0.5f;

		PointsAndOcclusion result;
		result.p000 = glm::vec3(nx, ny, nz);
		result.p001 = glm::vec3(nx, ny, pz);
		result.p010 = glm::vec3(nx, py, nz);
		result.p011 = glm::vec3(nx, py, pz);
		result.p100 = glm::vec3(px, ny, nz);
		result.p101 = glm::vec3(px, ny, pz);
		result.p110 = glm::vec3(px, py, nz);
		result.p111 = glm::vec3(px, py, pz);

		// Apply element rotation if present
		const auto& rotation = textureInfo.elemRotation;
		if (rotation.Angle != 0.0f && !rotation.Axis.empty())
		{
			// Convert origin from MC units (0-16) to local [-0.5 .. +0.5] space
			glm::vec3 origin(
				rotation.Origin.x / 16.0f - 0.5f, rotation.Origin.y / 16.0f - 0.5f, rotation.Origin.z / 16.0f - 0.5f);

			glm::vec3 axis(0.0f);
			if (rotation.Axis == "x")
				axis = {1, 0, 0};
			else if (rotation.Axis == "y")
				axis = {0, 1, 0};
			else if (rotation.Axis == "z")
				axis = {0, 0, 1};

			float radians = glm::radians(rotation.Angle);
			glm::mat4 rot = glm::rotate(glm::mat4(1.0f), radians, axis);

			auto rotatePoint = [&](glm::vec3& p)
			{
				glm::vec3 local = p - origin;
				local = glm::vec3(rot * glm::vec4(local, 0.0f));
				p = local + origin;
			};

			rotatePoint(result.p000);
			rotatePoint(result.p001);
			rotatePoint(result.p010);
			rotatePoint(result.p011);
			rotatePoint(result.p100);
			rotatePoint(result.p101);
			rotatePoint(result.p110);
			rotatePoint(result.p111);

			if (rotation.Rescale)
			{
				float scale = 1.0f / std::cos(radians);

				glm::vec3 scaleAxes(1.0f);
				if (rotation.Axis == "x")
					scaleAxes = {1.0f, scale, scale};
				else if (rotation.Axis == "y")
					scaleAxes = {scale, 1.0f, scale};
				else if (rotation.Axis == "z")
					scaleAxes = {scale, scale, 1.0f};

				auto scalePoint = [&](glm::vec3& p)
				{
					glm::vec3 local = p - origin;
					local *= scaleAxes;
					p = local + origin;
				};

				scalePoint(result.p000);
				scalePoint(result.p001);
				scalePoint(result.p010);
				scalePoint(result.p011);
				scalePoint(result.p100);
				scalePoint(result.p101);
				scalePoint(result.p110);
				scalePoint(result.p111);
			}
		}

		return result;
	}

	MeshBuilder::PointsAndOcclusion MeshBuilder::GetPointsAndOcclusion(
		SubChunkMesh* mesh, const int lx, const int wy, const int lz, const TextureInfo& textureInfo)
	{
		PointsAndOcclusion result;

		constexpr int subBlockSize = 32; // 2 sub-units per MC unit; supports 0.5-step precision and negative offsets

		float ofnx = textureInfo.from.x * 2.0f;
		float ofpx = textureInfo.to.x * 2.0f;

		float ofny = textureInfo.from.y * 2.0f;
		float ofpy = textureInfo.to.y * 2.0f;

		float ofnz = textureInfo.from.z * 2.0f;
		float ofpz = textureInfo.to.z * 2.0f;

		result.p000 = glm::vec3(lx * subBlockSize + ofnx, wy * subBlockSize + ofny, lz * subBlockSize + ofnz);
		result.p001 = glm::vec3(lx * subBlockSize + ofnx, wy * subBlockSize + ofny, lz * subBlockSize + ofpz);
		result.p010 = glm::vec3(lx * subBlockSize + ofnx, wy * subBlockSize + ofpy, lz * subBlockSize + ofnz);
		result.p011 = glm::vec3(lx * subBlockSize + ofnx, wy * subBlockSize + ofpy, lz * subBlockSize + ofpz);

		result.p100 = glm::vec3(lx * subBlockSize + ofpx, wy * subBlockSize + ofny, lz * subBlockSize + ofnz);
		result.p101 = glm::vec3(lx * subBlockSize + ofpx, wy * subBlockSize + ofny, lz * subBlockSize + ofpz);
		result.p110 = glm::vec3(lx * subBlockSize + ofpx, wy * subBlockSize + ofpy, lz * subBlockSize + ofnz);
		result.p111 = glm::vec3(lx * subBlockSize + ofpx, wy * subBlockSize + ofpy, lz * subBlockSize + ofpz);

		// Apply element rotation if present
		const auto& rotation = textureInfo.elemRotation;
		if (rotation.Angle != 0.0f && !rotation.Axis.empty())
		{
			// Convert origin from MC units (0-16) to sub-block units, relative to this block's world position
			glm::vec3 origin(lx * subBlockSize + rotation.Origin.x * 2.0f,
							 wy * subBlockSize + rotation.Origin.y * 2.0f,
							 lz * subBlockSize + rotation.Origin.z * 2.0f);

			glm::vec3 axis(0.0f);
			if (rotation.Axis == "x")
				axis = {1, 0, 0};
			else if (rotation.Axis == "y")
				axis = {0, 1, 0};
			else if (rotation.Axis == "z")
				axis = {0, 0, 1};

			float radians = glm::radians(rotation.Angle);
			glm::mat4 rot = glm::rotate(glm::mat4(1.0f), radians, axis);

			auto rotatePoint = [&](glm::vec3& p)
			{
				glm::vec3 local = p - origin;
				local = glm::vec3(rot * glm::vec4(local, 0.0f));
				p = local + origin;
			};

			rotatePoint(result.p000);
			rotatePoint(result.p001);
			rotatePoint(result.p010);
			rotatePoint(result.p011);
			rotatePoint(result.p100);
			rotatePoint(result.p101);
			rotatePoint(result.p110);
			rotatePoint(result.p111);

			// Apply rescale: expand the two axes perpendicular to the rotation axis so
			// the rotated element still fills the full block boundary (scale = 1/cos(angle)).
			if (rotation.Rescale)
			{
				float scale = 1.0f / std::cos(radians);

				glm::vec3 scaleAxes(1.0f);
				if (rotation.Axis == "x")
					scaleAxes = {1.0f, scale, scale};
				else if (rotation.Axis == "y")
					scaleAxes = {scale, 1.0f, scale};
				else if (rotation.Axis == "z")
					scaleAxes = {scale, scale, 1.0f};

				auto scalePoint = [&](glm::vec3& p)
				{
					glm::vec3 local = p - origin;
					local *= scaleAxes;
					p = local + origin;
				};

				scalePoint(result.p000);
				scalePoint(result.p001);
				scalePoint(result.p010);
				scalePoint(result.p011);
				scalePoint(result.p100);
				scalePoint(result.p101);
				scalePoint(result.p110);
				scalePoint(result.p111);
			}
		}

		// Fetch occlusion values from the mesh's occlusion map if this texture requires shading
		if (textureInfo.shade)
		{
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
		}

		return result;
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

} // namespace onion::voxel
