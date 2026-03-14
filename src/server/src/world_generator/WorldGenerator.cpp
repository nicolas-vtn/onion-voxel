#include "WorldGenerator.hpp"

#include <iostream>

#include <shared/utils/Stopwatch.hpp>
#include <shared/utils/Utils.hpp>

constexpr int BIOME_CELL_SIZE = 1024; // blocks

inline uint32_t Hash(int x, int z)
{
	uint32_t h = x * 374761393u + z * 668265263u;
	h = (h ^ (h >> 13)) * 1274126177u;
	return h ^ (h >> 16);
}

inline float HashFloat(uint32_t h)
{
	return (h & 0xFFFFFF) / float(0xFFFFFF);
}

glm::vec2 GetSeedPosition(int cellX, int cellZ)
{
	uint32_t h = Hash(cellX, cellZ);

	float rx = HashFloat(h);
	float rz = HashFloat(h * 48271u);

	return {cellX * BIOME_CELL_SIZE + rx * BIOME_CELL_SIZE, cellZ * BIOME_CELL_SIZE + rz * BIOME_CELL_SIZE};
}

namespace onion::voxel
{
	Biome GetSeedBiome(int cellX, int cellZ)
	{
		uint32_t h = Hash(cellX, cellZ);

		switch (h % 6)
		{
			case 0:
				return Biome::Ocean;
			case 1:
				return Biome::Plains;
			case 2:
				return Biome::Forest;
			case 3:
				return Biome::Desert;
			case 4:
				return Biome::Mountain;
			case 5:
				return Biome::Snow;
			default:
				return Biome::Ocean;
		}
	}

	static WorldGenerator::BiomeBlend GetBiomeBlend(float x, float z)
	{
		int cellX = static_cast<int>(std::floor(x / BIOME_CELL_SIZE));
		int cellZ = static_cast<int>(std::floor(z / BIOME_CELL_SIZE));

		WorldGenerator::BiomeSeed seeds[3] = {
			{FLT_MAX, Biome::Plains}, {FLT_MAX, Biome::Plains}, {FLT_MAX, Biome::Plains}};

		for (int dz = -1; dz <= 1; dz++)
		{
			for (int dx = -1; dx <= 1; dx++)
			{
				int cx = cellX + dx;
				int cz = cellZ + dz;

				glm::vec2 seed = GetSeedPosition(cx, cz);

				float dxs = seed.x - x;
				float dzs = seed.y - z;

				float dist = sqrt(dxs * dxs + dzs * dzs);

				Biome biome = GetSeedBiome(cx, cz);

				if (dist < seeds[0].dist)
				{
					seeds[2] = seeds[1];
					seeds[1] = seeds[0];
					seeds[0] = {dist, biome};
				}
				else if (dist < seeds[1].dist)
				{
					seeds[2] = seeds[1];
					seeds[1] = {dist, biome};
				}
				else if (dist < seeds[2].dist)
				{
					seeds[2] = {dist, biome};
				}
			}
		}

		float k = 0.02f;

		float w0 = exp(-seeds[0].dist * k);
		float w1 = exp(-seeds[1].dist * k);
		float w2 = exp(-seeds[2].dist * k);

		float sum = w0 + w1 + w2;

		WorldGenerator::BiomeBlend result;

		result.seeds[0] = seeds[0];
		result.seeds[1] = seeds[1];
		result.seeds[2] = seeds[2];

		result.weights[0] = w0 / sum;
		result.weights[1] = w1 / sum;
		result.weights[2] = w2 / sum;

		return result;
	}

	float GetBiomeHeight(Biome biome)
	{
		switch (biome)
		{
			case Biome::Ocean:
				return 1.f;
			case Biome::Plains:
				return 20.f;
			case Biome::Forest:
				return 40.f;
			case Biome::Desert:
				return 30.f;
			case Biome::Mountain:
				return 60.f;
			case Biome::Snow:
				return 25.f;
			default:
				return 70.0f;
		}
	}

	WorldGenerator::WorldGenerator(std::shared_ptr<WorldManager> worldManager) : m_WorldManager(worldManager)
	{
		SubscribeToWorldManagerEvents();
		Handle_SeedChanged(m_WorldManager->GetSeed());
	}

	WorldGenerator::~WorldGenerator()
	{
		m_ThreadPool.Close();
		m_WorldManagerEventHandles.clear();
	}

	void WorldGenerator::GenerateChunkAsync(const glm::ivec2& chunkPosition)
	{
		m_ThreadPool.Dispatch(
			[this, chunkPosition]()
			{
				if (!m_WorldManager->IsChunkLoaded(chunkPosition))
				{
					GenChunk genChunk = GenerateChunk(chunkPosition);
					if (genChunk.chunk)
					{
						m_WorldManager->AddChunk(genChunk.chunk, genChunk.outOfBoundsBlocks);
					}
				}
			});
	}

	void WorldGenerator::GenerateChunksAsync(const std::vector<glm::ivec2>& chunkPositions)
	{
		for (const auto& chunkPosition : chunkPositions)
		{
			GenerateChunkAsync(chunkPosition);
		}
	}

	uint32_t WorldGenerator::GetSeed() const
	{
		return m_WorldManager->GetSeed();
	}

	void WorldGenerator::SubscribeToWorldManagerEvents()
	{
		m_WorldManagerEventHandles.push_back(
			m_WorldManager->SeedChanged.Subscribe([this](const uint32_t& newSeed) { Handle_SeedChanged(newSeed); }));
	}

	void WorldGenerator::Handle_SeedChanged(const uint32_t& newSeed)
	{
		(void) newSeed; // Unused parameter
		ConfigureNoiseGenerators();
		m_SeededRandom.SetSeed(newSeed);
	}

	WorldGenerator::GenChunk WorldGenerator::GenerateChunk(const glm::ivec2& chunkPosition)
	{
		Stopwatch stopwatch;
		stopwatch.Start();

		GenChunk genChunk;

		switch (m_WorldGenerationType)
		{
			case eWorldGenerationType::DemoBlocks:
				genChunk = GenerateChunk_DemoBlocks(chunkPosition);
				break;
			case eWorldGenerationType::Superflat:
				genChunk = GenerateChunk_Superflat(chunkPosition);
				break;
			case eWorldGenerationType::Classic:
				genChunk = GenerateChunk_Classic(chunkPosition);
				break;
			case eWorldGenerationType::BiomeVisualizer:
				genChunk = GenerateChunk_BiomeVisualizer(chunkPosition);
				break;
			default:
				throw std::runtime_error("Invalid world generation type");
		}

		double elapsedMs = stopwatch.ElapsedMs();
		(void) elapsedMs; // Currently unused
		//std::cout << elapsedMs << " ms" << std::endl;

		return genChunk;
	}

	struct OrientationPair
	{
		BlockState::Orientation facing;
		BlockState::Orientation top;
	};

	static std::vector<OrientationPair> GetOrientationPairs(BlockId id)
	{
		using O = BlockState::Orientation;

		std::vector<OrientationPair> pairs;
		pairs.reserve(6);

		switch (BlockState::GetRotationType(id))
		{
			case BlockState::RotationType::None:
				{
					pairs.push_back({O::None, O::None});
					break;
				}

			case BlockState::RotationType::Facing:
				{
					pairs.push_back({O::North, O::Up});
					pairs.push_back({O::South, O::Up});
					pairs.push_back({O::East, O::Up});
					pairs.push_back({O::West, O::Up});
					break;
				}

			case BlockState::RotationType::Pillar:
				{
					pairs.push_back({O::Up, O::North});
					pairs.push_back({O::Down, O::North});

					pairs.push_back({O::North, O::Up});
					pairs.push_back({O::South, O::Up});
					pairs.push_back({O::East, O::Up});
					pairs.push_back({O::West, O::Up});
					break;
				}

			case BlockState::RotationType::Horizontal:
				{
					pairs.push_back({O::North, O::Up});
					pairs.push_back({O::South, O::Up});
					pairs.push_back({O::East, O::Up});
					pairs.push_back({O::West, O::Up});
					break;
				}
		}

		return pairs;
	}

	WorldGenerator::GenChunk WorldGenerator::GenerateChunk_DemoBlocks(const glm::ivec2& chunkPosition)
	{
		GenChunk genChunk;
		genChunk.chunk = std::make_shared<Chunk>(chunkPosition);
		auto& chunk = genChunk.chunk;

		constexpr int spacing = 3; // 1 block + 1 air gap
		constexpr int start = 1;   // avoid borders

		// ---- All orientations except None ----
		std::vector<BlockState::Orientation> orientations = {BlockState::Orientation::Up,
															 BlockState::Orientation::Down,
															 BlockState::Orientation::North,
															 BlockState::Orientation::South,
															 BlockState::Orientation::East,
															 BlockState::Orientation::West};

		// ---- Build valid (Facing, Top) pairs ----

		std::vector<OrientationPair> orientationPairs;

		auto IsPerpendicular = [](BlockState::Orientation a, BlockState::Orientation b)
		{
			if (a == b)
				return false;

			auto opposite = [](BlockState::Orientation o)
			{
				switch (o)
				{
					case BlockState::Orientation::Up:
						return BlockState::Orientation::Down;
					case BlockState::Orientation::Down:
						return BlockState::Orientation::Up;
					case BlockState::Orientation::North:
						return BlockState::Orientation::South;
					case BlockState::Orientation::South:
						return BlockState::Orientation::North;
					case BlockState::Orientation::East:
						return BlockState::Orientation::West;
					case BlockState::Orientation::West:
						return BlockState::Orientation::East;
					default:
						return BlockState::Orientation::None;
				}
			};

			return opposite(a) != b;
		};

		for (auto facing : orientations)
			for (auto top : orientations)
				if (IsPerpendicular(facing, top))
					orientationPairs.push_back({facing, top});

		int x = start;
		int z = start;
		int y = start;

		const int max = WorldConstants::CHUNK_SIZE - 1;

		// ---- Iterate all blocks ----
		const int blockIdCount = GetBlockIdCount();
		for (int blockId = 0; blockId < blockIdCount; blockId++)
		{
			BlockId id = static_cast<BlockId>(blockId);

			if (id == BlockId::Air)
				continue;

			auto pairs = GetOrientationPairs(id);
			for (const auto& pair : pairs)
			{
				if (x >= max)
				{
					x = start;
					z += spacing;
				}

				if (z >= max)
				{
					z = start;
					y += spacing;
				}

				if (y >= 100)
					return genChunk;

				BlockState block{id};
				block.Facing = pair.facing;
				block.Top = pair.top;

				chunk->SetBlock({x, y, z}, block);

				x += spacing;
			}
		}

		return genChunk;
	}

	WorldGenerator::GenChunk WorldGenerator::GenerateChunk_Superflat(const glm::ivec2& chunkPosition)
	{
		GenChunk genChunk;
		genChunk.chunk = std::make_shared<Chunk>(chunkPosition);
		auto& chunk = genChunk.chunk;

		constexpr bool GENERATE_GRASS = true;
		constexpr bool GENERATE_FLOWERS = true;
		constexpr bool GENERATE_TREES = false;

		int y = 0;

		// Bedrock layer
		BlockState bedrock{BlockId::Bedrock};
		for (y = 0; y <= 0; y++)
		{

			for (int x = 0; x < WorldConstants::CHUNK_SIZE; x++)
			{
				for (int z = 0; z < WorldConstants::CHUNK_SIZE; z++)
				{
					chunk->SetBlock(glm::ivec3(x, y, z), bedrock);
				}
			}
		}

		// 3 Dirt layers
		BlockState dirt{BlockId::Dirt};
		for (y = 1; y <= 3; y++)
		{
			for (int x = 0; x < WorldConstants::CHUNK_SIZE; x++)
			{
				for (int z = 0; z < WorldConstants::CHUNK_SIZE; z++)
				{
					chunk->SetBlock(glm::ivec3(x, y, z), dirt);
				}
			}
		}

		// Grass layer
		BlockState grass{BlockId::Grass};
		for (y = 4; y <= 4; y++)
		{
			for (int x = 0; x < WorldConstants::CHUNK_SIZE; x++)
			{
				for (int z = 0; z < WorldConstants::CHUNK_SIZE; z++)
				{
					chunk->SetBlock(glm::ivec3(x, y, z), grass);
				}
			}
		}

		int height = 4; // Grass layer height
		for (int z = 0; z < WorldConstants::CHUNK_SIZE; z++)
		{
			for (int x = 0; x < WorldConstants::CHUNK_SIZE; x++)
			{
				glm::ivec3 worldPos = {(chunkPosition.x * WorldConstants::CHUNK_SIZE) + x,
									   height, // Grass layer height
									   (chunkPosition.y * WorldConstants::CHUNK_SIZE) + z};

				bool shouldGenerateShortGrass = ShouldGenerateShortGrass(worldPos);
				if (GENERATE_GRASS && shouldGenerateShortGrass)
				{
					chunk->SetBlock(glm::ivec3(x, height + 1, z), BlockState(BlockId::ShortGrass));
				}

				bool shouldGenerateFlower = ShouldGenerateFlower(worldPos);
				if (GENERATE_FLOWERS && shouldGenerateFlower)
				{
					BlockId flowerId = GetFlowerType(worldPos);
					chunk->SetBlock(glm::ivec3(x, height + 1, z), BlockState(flowerId));
				}

				bool shouldGenerateTree = ShouldGenerateTree(worldPos);
				if (GENERATE_TREES && shouldGenerateTree)
				{
					double val = m_SeededRandom.GetValue(worldPos + glm::ivec3(25, 2584, 88));

					BlockId logId = (val < 0.025) ? BlockId::BirchLog : BlockId::OakLog; // 2.5% chance for birch
					BlockId leavesId = (logId == BlockId::BirchLog) ? BlockId::BirchLeaves : BlockId::OakLeaves;

					// Tree height between 2 and 6
					int treeHeight = 2 + (m_SeededRandom.GetValue(worldPos + glm::ivec3(88, 654, 2584)) * 4);

					glm::ivec3 treeBlock = worldPos + glm::ivec3(0, 1, 0);
					Schematic tree = GenerateTree(
						treeBlock, treeHeight, logId, leavesId); // Generate a tree at the top block position
					MergeSchematicInChunk(tree, genChunk);
				}
			}
		}

		return genChunk;
	}

	WorldGenerator::GenChunk WorldGenerator::GenerateChunk_Classic(const glm::ivec2& chunkPosition)
	{
		GenChunk genChunk;

		// Creates the Chunk
		genChunk.chunk = std::make_shared<Chunk>(chunkPosition, 100);
		auto& chunk = genChunk.chunk;

		constexpr int CHUNK_SIZE = WorldConstants::CHUNK_SIZE;

		// Gets the height map
		uint16_t heightMap[CHUNK_SIZE][CHUNK_SIZE] = {0};
		for (uint8_t z = 0; z < CHUNK_SIZE; z++)
		{
			for (uint8_t x = 0; x < CHUNK_SIZE; x++)
			{
				int realWorldX = (chunkPosition.x * CHUNK_SIZE + x);
				int realWorldZ = (chunkPosition.y * CHUNK_SIZE + z);

				float noisePositionX = static_cast<float>(realWorldX) * m_SmoothnessX;
				float noisePositionZ = static_cast<float>(realWorldZ) * m_SmoothnessZ;

				float continents = GetFractalNoise(m_NoiseContinent, realWorldX, realWorldZ, 3, 2.0f, 0.5f);

				float mountains =
					std::max(0.0f, GetFractalNoise(m_NoiseMountain, realWorldX, realWorldZ, 4, 2.0f, 0.5f));

				float detail = GetFractalNoise(m_NoiseDetail, realWorldX, realWorldZ, 3, 2.0f, 0.5f);

				float continentMask = (continents + 1.0f) * 0.5f; // [-1,1] -> [0,1]
				continentMask = std::pow(continentMask, 1.5f);

				float height = m_SeaLevel + continents * 100.0f + (mountains * continentMask) * 180.0f + detail * 5.0f;

				heightMap[x][z] =
					static_cast<uint16_t>(std::clamp(height, 1.0f, static_cast<float>(m_WorldHeight - 1)));

				//float h = GetFractalNoise(noisePositionX, noisePositionZ);
				//heightMap[x][z] = GetTerrainHeight(height);
			}
		}

		// Fills the chunks with blocks based on the height map
		for (uint8_t z = 0; z < CHUNK_SIZE; z++)
		{
			for (uint8_t x = 0; x < CHUNK_SIZE; x++)
			{
				uint16_t height = heightMap[x][z];

				// Higher than sea level
				if (height >= m_SeaLevel)
				{
					BlockId topBlockId = (height >= m_SnowLevel) ? BlockId::SnowGrass : BlockId::Grass;
					for (uint16_t y = 0; y <= height + 1; y++)
					{
						// Bedrock
						if (y == 0)
						{
							chunk->SetBlock_Unsafe(
								(uint8_t) x, y, (uint8_t) z, BlockId::Bedrock); // Set bedrock at the bottom
							continue;
						}

						float altitudeFactor =
							std::clamp((float) (height - m_SeaLevel) / (float) (m_SnowLevel - m_SeaLevel), 0.0f, 1.0f);
						int numDirtLayers = (int) std::round((1.0f - altitudeFactor) * 3.0f);

						// Stone layers
						if (y < height - numDirtLayers)
						{
							chunk->SetBlock_Unsafe(
								(uint8_t) x, y, (uint8_t) z, BlockId::Stone); // Set stone below the dirt
							continue;
						}

						// Dirt layers
						if (y < height)
						{
							chunk->SetBlock_Unsafe(
								(uint8_t) x, y, (uint8_t) z, BlockId::Dirt); // Set dirt below the top block
							continue;
						}

						if (y == height)
						{
							chunk->SetBlock_Unsafe(
								(uint8_t) x, y, (uint8_t) z, topBlockId); // Set the Top block (grass or snow grass)
							continue;
						}

						if (y == height + 1 && y < m_SnowLevel)
						{
							glm::ivec3 currentWorldPos = {
								chunkPosition.x * CHUNK_SIZE + x, y, chunkPosition.y * CHUNK_SIZE + z};
							if (ShouldGenerateShortGrass(currentWorldPos))
							{
								chunk->SetBlock_Unsafe((uint8_t) x, y, (uint8_t) z, BlockId::ShortGrass);
							}
							if (ShouldGenerateFlower(currentWorldPos))
							{
								BlockId flowerId = GetFlowerType(currentWorldPos);
								chunk->SetBlock_Unsafe((uint8_t) x, y, (uint8_t) z, flowerId);
							}
						}
					}
				}

				// Lower than sea level
				if (height < m_SeaLevel)
				{
					for (uint16_t y = 0; y < m_WorldHeight; y++)
					{
						// Bedrock
						if (y == 0)
						{
							chunk->SetBlock_Unsafe(
								(uint8_t) x, y, (uint8_t) z, BlockId::Bedrock); // Set bedrock at the bottom
						}
						// Stone
						else if (y < height - 3)
						{
							chunk->SetBlock_Unsafe(
								(uint8_t) x, y, (uint8_t) z, BlockId::Stone); // Set stone at the bottom
						}
						// Gravel Or Sand
						else if (y <= height)
						{
							// Deeper sea has gravel, shallower sea has sand
							BlockId seaFloor = (height < m_SeaLevel - 5) ? BlockId::Gravel : BlockId::Sand;
							chunk->SetBlock_Unsafe((uint8_t) x, y, (uint8_t) z, seaFloor);
						}
						else if (y > height && y < m_SeaLevel)
						{
							chunk->SetBlock_Unsafe(
								(uint8_t) x, y, (uint8_t) z, BlockId::Water); // Set water above the gravel
						}
					}
				}
			}
		}

		// Adds trees
		for (uint8_t x = 0; x < WorldConstants::CHUNK_SIZE; x++)
		{
			for (uint8_t z = 0; z < WorldConstants::CHUNK_SIZE; z++)
			{
				int realWorldX = (chunkPosition.x * WorldConstants::CHUNK_SIZE + x);
				int realWorldZ = (chunkPosition.y * WorldConstants::CHUNK_SIZE + z);
				int height = heightMap[x][z];

				glm::ivec3 worldPos = {realWorldX, height, realWorldZ};

				if (height < m_SeaLevel)
					continue; // Skip if the height is below sea level (no trees in water)

				bool shouldGenerateTree = ShouldGenerateTree(worldPos);
				if (shouldGenerateTree)
				{
					glm::ivec3 treeBlock = worldPos + glm::ivec3(0, 1, 0);

					// Sets a dirtblock under the tree if the top block is grass
					BlockState bottomBlock = chunk->GetBlock({x, height, z});
					if (bottomBlock.ID == BlockId::Grass)
					{
						chunk->SetBlock({x, height, z}, BlockId::Dirt);
					}

					double val = m_SeededRandom.GetValue(worldPos + glm::ivec3(25, 2584, 88));
					BlockId logId = (val < 0.025) ? BlockId::BirchLog : BlockId::OakLog; // 2.5% chance for birch
					BlockId leavesId = (logId == BlockId::BirchLog) ? BlockId::BirchLeaves : BlockId::OakLeaves;
					// Tree height between 2 and 6
					int treeHeight = 2 + (m_SeededRandom.GetValue(worldPos + glm::ivec3(88, 654, 2584)) * 4);

					Schematic tree = GenerateTree(treeBlock, treeHeight, logId, leavesId);
					std::unordered_set<BlockId> overwritables;
					overwritables.insert(BlockId::Air);
					overwritables.insert(BlockId::ShortGrass);
					for (const auto& blockId : BlockState::Flowers)
					{
						overwritables.insert(blockId);
					}

					MergeSchematicInChunk(tree, genChunk, overwritables);
				}
			}
		}

		// Optimize the chunk (less memory, faster to send to clients)
		chunk->Optimize();

		return genChunk;
	}

	WorldGenerator::GenChunk WorldGenerator::GenerateChunk_BiomeVisualizer(const glm::ivec2& chunkPosition)
	{
		GenChunk genChunk;
		genChunk.chunk = std::make_shared<Chunk>(chunkPosition);
		auto& chunk = genChunk.chunk;

		WorldGenerator::BiomeBlend biomeBlend;
		for (int x = 0; x < WorldConstants::CHUNK_SIZE; x++)
		{
			for (int z = 0; z < WorldConstants::CHUNK_SIZE; z++)
			{
				glm::ivec3 worldPos = {(chunkPosition.x * WorldConstants::CHUNK_SIZE) + x,
									   0, // Y doesn't matter for biome visualization
									   (chunkPosition.y * WorldConstants::CHUNK_SIZE) + z};
				biomeBlend = GetBiome(worldPos);

				BlockId blockId;
				switch (biomeBlend.seeds[0].biome)
				{
					case Biome::Plains:
						blockId = BlockId::Grass;
						break;
					case Biome::Desert:
						blockId = BlockId::Sand;
						break;
					case Biome::Mountain:
						blockId = BlockId::Stone;
						break;
					case Biome::Forest:
						blockId = BlockId::OakLog;
						break;
					case Biome::Snow:
						blockId = BlockId::SnowGrass;
						break;
					case Biome::Ocean:
						blockId = BlockId::Water;
						break;

					default:
						blockId = BlockId::Dirt;
						break;
				}

				float h0 = GetBiomeHeight(biomeBlend.seeds[0].biome);
				float h1 = GetBiomeHeight(biomeBlend.seeds[1].biome);
				float h2 = GetBiomeHeight(biomeBlend.seeds[2].biome);

				float height = biomeBlend.weights[0] * h0 + biomeBlend.weights[1] * h1 + biomeBlend.weights[2] * h2;

				for (int y = 0; y < height; y++)
				{
					chunk->SetBlock(glm::ivec3(x, y, z), blockId);
				}
			}
		}

		return genChunk;
	}

	bool WorldGenerator::ShouldGenerateTree(const glm::ivec3& position) const
	{

		if (position.y > m_SnowLevel)
		{
			return false; // Don't generate trees above the snow level
		}

		double randomVal = m_SeededRandom.GetValue(position);

		// Less trees at higher altitudes
		double altitudeFactor = static_cast<double>(position.y) / static_cast<double>(m_SnowLevel);

		constexpr double BASE_TREE_CHANCE = 0.02; // Base 2% chance to generate a tree

		randomVal += altitudeFactor * BASE_TREE_CHANCE; // Increase the random value slightly based on altitude

		return randomVal < BASE_TREE_CHANCE;
	}

	bool WorldGenerator::ShouldGenerateShortGrass(const glm::ivec3& position) const
	{
		if (position.y > m_SnowLevel)
		{
			return false; // Don't generate short grass above the snow level
		}

		double randomVal = m_SeededRandom.GetValue(position);

		// Less short grass at higher altitudes
		double altitudeFactor = static_cast<double>(position.y) / static_cast<double>(m_SnowLevel);

		constexpr double BASE_SHORT_GRASS_CHANCE = 0.25; // Base 25% chance to generate short grass

		randomVal += altitudeFactor * BASE_SHORT_GRASS_CHANCE; // Increase the random value slightly based on altitude

		return randomVal < BASE_SHORT_GRASS_CHANCE;
	}

	bool WorldGenerator::ShouldGenerateFlower(const glm::ivec3& position) const
	{
		if (position.y > m_SnowLevel)
		{
			return false; // Don't generate flowers above the snow level
		}

		double randomVal = m_SeededRandom.GetValue(position);

		// Less flowers at higher altitudes
		double altitudeFactor = static_cast<double>(position.y) / static_cast<double>(m_SnowLevel);

		constexpr double BASE_FLOWER_CHANCE = 0.05; // Base 5% chance to generate flowers

		randomVal += altitudeFactor * BASE_FLOWER_CHANCE; // Increase the random value slightly based on altitude
		return randomVal < BASE_FLOWER_CHANCE;
	}

	BlockId WorldGenerator::GetFlowerType(const glm::ivec3& position) const
	{
		const double val = m_SeededRandom.GetValue(position);
		const double flowerVal = m_SeededRandom.GetValue(val);
		const size_t index = static_cast<size_t>(flowerVal * BlockState::Flowers.size());
		return BlockState::Flowers[index];
	}

	WorldGenerator::BiomeBlend WorldGenerator::GetBiome(const glm::ivec3& pos) const
	{
		float noiseX = GetFractalNoise(m_NoiseWarp, pos.x, pos.z, 4, 2.0f, 0.5f) * 200.f;
		float noiseZ = GetFractalNoise(m_NoiseWarp2, pos.z, pos.x, 4, 2.0f, 0.5f) * 200.f;

		float x = pos.x + noiseX;
		float z = pos.z + noiseZ;

		return GetBiomeBlend(x, z);
	}

	void WorldGenerator::MergeSchematicInChunk(const Schematic& schematic,
											   GenChunk& genChunk,
											   const std::unordered_set<BlockId>& overwritables)
	{
		const auto& chunk = genChunk.chunk;
		const auto chunkPosition = chunk->GetPosition();

		const auto schematicOrigin = schematic.GetOrigin();
		const auto schematicSize = schematic.GetSize();

		for (int x = 0; x < schematicSize.x; x++)
		{
			for (int y = 0; y < schematicSize.y; y++)
			{
				for (int z = 0; z < schematicSize.z; z++)
				{

					BlockState block = schematic.GetBlock({x, y, z});
					const glm::ivec3 blockWorldPos{schematicOrigin.x + x, schematicOrigin.y + y, schematicOrigin.z + z};

					// Skip air blocks
					if (block.ID == BlockId::Air)
					{
						continue;
					}

					// Check if the block is in the chunk
					const auto blockChunkPos = Utils::WorldToChunkPosition(blockWorldPos);
					if (blockChunkPos == chunkPosition)
					{
						// Get local coordinates in the chunk
						const glm::ivec3 localPos = Utils::WorldToLocalPosition(blockWorldPos);
						BlockId existingBlockId = chunk->GetBlock(localPos).ID;
						if (overwritables.contains(existingBlockId) || existingBlockId == BlockId::Air)
						{
							// The block can be overwritten, set it in the chunk
							chunk->SetBlock(localPos, block);
						}
					}
					else
					{
						// Add the block to the out-of-bounds blocks
						genChunk.outOfBoundsBlocks.emplace_back(blockWorldPos, block);
					}
				}
			}
		}
	}

	Schematic
	WorldGenerator::GenerateTree(const glm::ivec3& position, int height, BlockId logType, BlockId leavesType) const
	{
		glm::ivec3 treePosition{position.x, position.y, position.z};

		glm::ivec3 SchematicOrigin{treePosition.x - 2, treePosition.y, treePosition.z - 2};

		Schematic treeSchematic({5, 10, 5}, SchematicOrigin);

		constexpr int MIN_TREE_HEIGHT = 2;

		int treeHeight = std::max(height, MIN_TREE_HEIGHT);

		static std::random_device rd;  // Non-deterministic (hardware RNG)
		static std::mt19937 gen(rd()); // Only seeded once
		static std::uniform_int_distribution<int> dist(1, 100);

		// Creates the block palette for the tree
		BlockState verticalLog = BlockState(logType, BlockState::Orientation::Up, BlockState::Orientation::North);
		BlockState leaves = BlockState(leavesType);

		// Generate the Logs
		for (int y = 0; y < treeHeight + 3; y++)
		{
			// Vertical log
			treeSchematic.SetBlock({2, y, 2}, verticalLog); // Set the trunk block
		}

		// Generate the Leaves
		for (int x = 0; x < 5; x++)
		{
			for (int z = 0; z < 5; z++)
			{
				for (int y = treeHeight; y < treeHeight + 2; y++)
				{
					if (treeSchematic.GetBlockId({x, y, z}) == BlockId::Air)
					{
						treeSchematic.SetBlock({x, y, z}, leaves); // Set the leaves block
					}

					// 20% chance to NOT have leaves at the corners
					if ((x == 0 && z == 0) || (x == 0 && z == 4) || (x == 4 && z == 0) || (x == 4 && z == 4))
					{
						if (dist(gen) <= 20)
						{
							// Set the corner leaves to air
							treeSchematic.SetBlock({x, y, z}, BlockState(BlockId::Air));
						}
					}
				}
			}
		}

		for (int x = 1; x < 4; x++)
		{
			for (int z = 1; z < 4; z++)
			{
				for (int y = treeHeight + 2; y <= treeHeight + 3; y++)
				{
					if (treeSchematic.GetBlockId({x, y, z}) == BlockId::Air)
					{
						treeSchematic.SetBlock({x, y, z}, leaves); // Set the leaves block
					}
				}
			}
		}

		return treeSchematic;
	}

	void WorldGenerator::ConfigureNoiseGenerators()
	{
		uint32_t seed = GetSeed();
		m_NoiseContinent.SetSeed(seed);
		m_NoiseContinent.SetNoiseType(m_NoiseType);
		m_NoiseContinent.SetFrequency(m_FrequencyContinent);

		m_NoiseMountain.SetSeed(seed + 1);
		m_NoiseMountain.SetNoiseType(m_NoiseType);
		m_NoiseMountain.SetFrequency(m_FrequencyMountain);

		m_NoiseDetail.SetSeed(seed + 2);
		m_NoiseDetail.SetNoiseType(m_NoiseType);
		m_NoiseDetail.SetFrequency(m_FrequencyDetail);

		m_NoiseTemperature.SetSeed(seed + 3);
		m_NoiseTemperature.SetNoiseType(m_NoiseType);
		m_NoiseTemperature.SetFrequency(m_FrequencyTemperature);

		m_NoiseHumidity.SetSeed(seed + 4);
		m_NoiseHumidity.SetNoiseType(m_NoiseType);
		m_NoiseHumidity.SetFrequency(m_FrequencyHumidity);

		m_NoiseWarp.SetSeed(seed + 5);
		m_NoiseWarp.SetNoiseType(m_NoiseType);
		m_NoiseWarp.SetFrequency(m_FrequencyWarp);

		m_NoiseWarp2.SetSeed(seed + 6);
		m_NoiseWarp2.SetNoiseType(m_NoiseType);
		m_NoiseWarp2.SetFrequency(m_FrequencyWarp);
	}

	float WorldGenerator::GetFractalNoise(
		const FastNoiseLite& noise, float x, float z, int octaves, float lacunarity, float gain) const
	{
		float amplitude = 1.0f;
		float frequency = 1.0f;

		float value = 0.0f;
		float maxValue = 0.0f;

		for (int i = 0; i < octaves; i++)
		{
			value += noise.GetNoise(x * frequency, z * frequency) * amplitude;
			maxValue += amplitude;

			frequency *= lacunarity;
			amplitude *= gain;
		}

		return (maxValue > 0.0f) ? (value / maxValue) : 0.0f;
	}

	constexpr float WorldGenerator::GetTerrainHeight(float noiseHeight) const
	{
		// Scale and shift the noise value to get a height between 0 and m_MaxTerrainHeight
		return (noiseHeight * (m_MaxTerrainHeight / 2)) + (m_MaxTerrainHeight / 2);
	}

} // namespace onion::voxel
