#include "WorldGenerator.hpp"

#include <iostream>

#include <shared/utils/Stopwatch.hpp>
#include <shared/utils/Utils.hpp>

namespace onion::voxel
{
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
		ConfigureNoiseGenerator();
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
		for (int blockId = 0; blockId < blockIdCount; ++blockId)
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
					glm::ivec3 treeBlock = worldPos + glm::ivec3(0, 1, 0);
					Schematic tree = GenerateTree(treeBlock); // Generate a tree at the top block position
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
		for (uint8_t z = 0; z < CHUNK_SIZE; ++z)
		{
			for (uint8_t x = 0; x < CHUNK_SIZE; ++x)
			{
				int realWorldX = (chunkPosition.x * CHUNK_SIZE + x);
				int realWorldZ = (chunkPosition.y * CHUNK_SIZE + z);

				float noisePositionX = static_cast<float>(realWorldX) * m_SmoothnessX;
				float noisePositionZ = static_cast<float>(realWorldZ) * m_SmoothnessZ;

				heightMap[x][z] =
					static_cast<uint16_t>(GetTerrainHeight(m_FastNoiseLite.GetNoise(noisePositionX, noisePositionZ)));
			}
		}

		// Fills the chunks with blocks based on the height map
		for (uint8_t z = 0; z < CHUNK_SIZE; ++z)
		{
			for (uint8_t x = 0; x < CHUNK_SIZE; ++x)
			{
				uint16_t height = heightMap[x][z];

				// Higher than sea level
				if (height >= m_AverageHeight)
				{
					for (uint16_t y = 0; y < m_WorldHeight; ++y)
					{
						//if (y == 0)
						//{
						//	chunk->SetBlock({x, y, z}, BlockId::Bedrock); // Set bedrock at the bottom
						//}
						//else if (y == height)
						//{
						//	chunk->SetBlock({x, y, z}, BlockId::Grass); // Set grass block at the top
						//}
						//else if (y == height - 1 || y == height - 2)
						//{
						//	chunk->SetBlock({x, y, z}, BlockId::Dirt); // Set dirt blocks below the grass
						//}
						//else if (y < height)
						//{
						//	chunk->SetBlock({x, y, z}, BlockId::Stone); // Set stone below the grass
						//}

						if (y == 0)
						{
							chunk->SetBlock_Unsafe(
								(uint8_t) x, (uint8_t) y, (uint8_t) z, BlockId::Bedrock); // Set bedrock at the bottom
						}
						else if (y == height)
						{
							chunk->SetBlock_Unsafe(
								(uint8_t) x, (uint8_t) y, (uint8_t) z, BlockId::Grass); // Set grass block at the top
						}
						else if (y == height + 1)
						{
							glm::ivec3 currentWorldPos = {
								chunkPosition.x * CHUNK_SIZE + x, y, chunkPosition.y * CHUNK_SIZE + z};
							if (ShouldGenerateShortGrass(currentWorldPos))
							{
								chunk->SetBlock_Unsafe((uint8_t) x, (uint8_t) y, (uint8_t) z, BlockId::ShortGrass);
							}
							if (ShouldGenerateFlower(currentWorldPos))
							{
								BlockId flowerId = GetFlowerType(currentWorldPos);
								chunk->SetBlock_Unsafe((uint8_t) x, (uint8_t) y, (uint8_t) z, flowerId);
							}
						}
						else if (y == height - 1 || y == height - 2)
						{
							chunk->SetBlock_Unsafe((uint8_t) x,
												   (uint8_t) y,
												   (uint8_t) z,
												   BlockId::Dirt); // Set dirt blocks below the grass
						}
						else if (y < height)
						{
							chunk->SetBlock_Unsafe(
								(uint8_t) x, (uint8_t) y, (uint8_t) z, BlockId::Stone); // Set stone below the grass
						}
					}
				}

				// Lower than sea level
				if (height < m_AverageHeight)
				{
					for (uint16_t y = 0; y < m_WorldHeight; ++y)
					{
						//if (y == 0)
						//{
						//	chunk->SetBlock({x, y, z}, BlockId::Bedrock); // Set bedrock at the bottom
						//}
						//else if (y == height || y == height - 1)
						//{
						//	chunk->SetBlock({x, y, z}, BlockId::Gravel); // Set gravel blocks below the sea level
						//}
						//else if (y < height)
						//{
						//	chunk->SetBlock({x, y, z}, BlockId::Stone); // Set stone below the gravel
						//}
						//else if (y > height && y < m_AverageHeight)
						//{
						//	chunk->SetBlock({x, y, z}, BlockId::Water); // Set water above the gravel
						//}

						if (y == 0)
						{
							chunk->SetBlock_Unsafe(
								(uint8_t) x, (uint8_t) y, (uint8_t) z, BlockId::Bedrock); // Set bedrock at the bottom
						}
						else if (y == height || y == height - 1)
						{
							chunk->SetBlock_Unsafe((uint8_t) x,
												   (uint8_t) y,
												   (uint8_t) z,
												   BlockId::Gravel); // Set gravel blocks below the sea level
						}
						else if (y < height)
						{
							chunk->SetBlock_Unsafe(
								(uint8_t) x, (uint8_t) y, (uint8_t) z, BlockId::Stone); // Set stone below the gravel
						}
						else if (y > height && y < m_AverageHeight)
						{
							chunk->SetBlock_Unsafe(
								(uint8_t) x, (uint8_t) y, (uint8_t) z, BlockId::Water); // Set water above the gravel
						}
					}
				}
			}
		}

		// Adds trees
		for (uint8_t x = 0; x < WorldConstants::CHUNK_SIZE; ++x)
		{
			for (uint8_t z = 0; z < WorldConstants::CHUNK_SIZE; ++z)
			{
				int realWorldX = (chunkPosition.x * WorldConstants::CHUNK_SIZE + x);
				int realWorldZ = (chunkPosition.y * WorldConstants::CHUNK_SIZE + z);
				int height = heightMap[x][z];

				glm::ivec3 worldPos = {realWorldX, height, realWorldZ};

				if (height < m_AverageHeight)
					continue; // Skip if the height is below average (no trees in water)

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

					Schematic tree = GenerateTree(treeBlock); // Generate a tree at the top block position
					MergeSchematicInChunk(tree, genChunk);
				}
			}
		}

		// Optimize the chunk (less memory, faster to send to clients)
		chunk->Optimize();

		return genChunk;
	}

	bool WorldGenerator::ShouldGenerateTree(const glm::ivec3& position) const
	{
		// 2% chance to generate a tree
		double randomVal = m_SeededRandom.GetValue(position);
		return randomVal < 0.02;
	}

	bool WorldGenerator::ShouldGenerateShortGrass(const glm::ivec3& position) const
	{
		// 25% chance to generate short grass
		double randomVal = m_SeededRandom.GetValue(position);
		return randomVal < 0.25;
	}

	bool WorldGenerator::ShouldGenerateFlower(const glm::ivec3& position) const
	{
		// 5% chance to generate a flower
		double randomVal = m_SeededRandom.GetValue(position);
		return randomVal < 0.05;
	}

	BlockId WorldGenerator::GetFlowerType(const glm::ivec3& position) const
	{
		const double val = m_SeededRandom.GetValue(position);
		const double flowerVal = m_SeededRandom.GetValue(val);
		const size_t index = static_cast<size_t>(flowerVal * BlockState::Flowers.size());
		return BlockState::Flowers[index];
	}

	void WorldGenerator::MergeSchematicInChunk(const Schematic& schematic, GenChunk& genChunk)
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
						// Set the block in the chunk pile

						// Get local coordinates in the chunk
						const glm::ivec3 localPos = Utils::WorldToLocalPosition(blockWorldPos);
						chunk->SetBlock(localPos, block);
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

	Schematic WorldGenerator::GenerateTree(const glm::ivec3& position) const
	{
		glm::ivec3 treePosition{position.x, position.y, position.z};

		glm::ivec3 SchematicOrigin{treePosition.x - 2, treePosition.y, treePosition.z - 2};

		Schematic treeSchematic({5, 10, 5}, SchematicOrigin);

		int minTreeHeight = 2;
		int maxTreeHeight = 5;

		// Combine seed, x and z in a simple way
		std::uint64_t combined = static_cast<std::uint64_t>(GetSeed()) ^
			(static_cast<std::uint64_t>(position.x) << 16) ^ (static_cast<std::uint64_t>(position.z) << 32);

		std::mt19937 generator(static_cast<std::mt19937::result_type>(combined));
		std::uniform_int_distribution<int> heightDistribution(minTreeHeight, maxTreeHeight);

		int treeHeight = heightDistribution(generator);

		static std::random_device rd;  // Non-deterministic (hardware RNG)
		static std::mt19937 gen(rd()); // Only seeded once
		static std::uniform_int_distribution<int> dist(1, 100);

		// Creates the block palette for the tree
		BlockState verticalOakLog =
			BlockState(BlockId::OakLog, BlockState::Orientation::Up, BlockState::Orientation::North);
		BlockState oakLeaves = BlockState(BlockId::OakLeaves);

		// Generate the Logs
		for (int y = 0; y < treeHeight + 3; ++y)
		{
			// Vertical oak log
			treeSchematic.SetBlock({2, y, 2}, verticalOakLog); // Set the trunk block
		}

		// Generate the Leaves
		for (int x = 0; x < 5; ++x)
		{
			for (int z = 0; z < 5; ++z)
			{
				for (int y = treeHeight; y < treeHeight + 2; y++)
				{
					if (treeSchematic.GetBlockId({x, y, z}) == BlockId::Air)
					{
						treeSchematic.SetBlock({x, y, z}, oakLeaves); // Set the leaves block
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

		for (int x = 1; x < 4; ++x)
		{
			for (int z = 1; z < 4; ++z)
			{
				for (int y = treeHeight + 2; y <= treeHeight + 3; y++)
				{
					if (treeSchematic.GetBlockId({x, y, z}) == BlockId::Air)
					{
						treeSchematic.SetBlock({x, y, z}, oakLeaves); // Set the leaves block
					}
				}
			}
		}

		return treeSchematic;
	}

	void WorldGenerator::ConfigureNoiseGenerator()
	{
		m_FastNoiseLite.SetSeed(GetSeed());
		m_FastNoiseLite.SetNoiseType(m_NoiseType);
		m_FastNoiseLite.SetFrequency(m_Frequency); // Smaller = smoother, larger = more rugged
	}

	constexpr float WorldGenerator::GetTerrainHeight(float noiseHeight) const
	{
		return (noiseHeight * m_Amplitude + m_AverageHeight);
	}

} // namespace onion::voxel
