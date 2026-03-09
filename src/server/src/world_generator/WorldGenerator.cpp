#include "WorldGenerator.hpp"

namespace onion::voxel
{
	WorldGenerator::WorldGenerator(std::shared_ptr<WorldManager> worldManager) : m_WorldManager(worldManager)
	{
		m_ThreadChunkGeneration = std::jthread([this](std::stop_token st) { ChunkGenerationThreadFunction(st); });

		SubscribeToWorldManagerEvents();
		Handle_SeedChanged(m_WorldManager->GetSeed());
	}

	WorldGenerator::~WorldGenerator()
	{
		if (m_ThreadChunkGeneration.joinable())
		{
			m_ThreadChunkGeneration.request_stop();
			m_ThreadChunkGeneration.join();
		}
	}

	void WorldGenerator::GenerateChunkAsync(const glm::ivec2& chunkPosition)
	{
		m_ChunkGenerationQueue.Push(chunkPosition);
	}

	void WorldGenerator::GenerateChunksAsync(const std::vector<glm::ivec2>& chunkPositions)
	{
		for (const auto& chunkPosition : chunkPositions)
		{
			m_ChunkGenerationQueue.Push(chunkPosition);
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
		ConfigureNoiseGenerator();
	}

	void WorldGenerator::ChunkGenerationThreadFunction(std::stop_token st)
	{
		while (!st.stop_requested())
		{
			glm::ivec2 chunkPosition;
			if (m_ChunkGenerationQueue.TryPop(chunkPosition))
			{
				if (!m_WorldManager->IsChunkLoaded(chunkPosition))
				{
					GenChunk genChunk = GenerateChunk(chunkPosition);
					if (genChunk.chunk)
					{
						m_WorldManager->AddChunk(genChunk.chunk);

						// TO DO : Add OutOfBoundsBlocks to WorldManager
					}
				}
			}
			else
			{
				std::this_thread::sleep_for(
					std::chrono::milliseconds(10)); // Sleep for a short duration to prevent busy waiting
			}
		}
	}

	WorldGenerator::GenChunk WorldGenerator::GenerateChunk(const glm::ivec2& chunkPosition)
	{
		switch (m_WorldGenerationType)
		{
			case eWorldGenerationType::DemoBlocks:
				return GenerateChunk_DemoBlocks(chunkPosition);
			case eWorldGenerationType::Superflat:
				return GenerateChunk_Superflat(chunkPosition);
			case eWorldGenerationType::Classic:
				return GenerateChunk_Classic(chunkPosition);
			default:
				throw std::runtime_error("Invalid world generation type");
		}
	}

	struct OrientationPair
	{
		Block::Orientation facing;
		Block::Orientation top;
	};

	static std::vector<OrientationPair> GetOrientationPairs(BlockId id)
	{
		using O = Block::Orientation;

		std::vector<OrientationPair> pairs;
		pairs.reserve(6);

		switch (Block::GetRotationType(id))
		{
			case Block::RotationType::None:
				{
					pairs.push_back({O::None, O::None});
					break;
				}

			case Block::RotationType::Facing:
				{
					pairs.push_back({O::North, O::Up});
					pairs.push_back({O::South, O::Up});
					pairs.push_back({O::East, O::Up});
					pairs.push_back({O::West, O::Up});
					break;
				}

			case Block::RotationType::Pillar:
				{
					pairs.push_back({O::Up, O::North});
					pairs.push_back({O::Down, O::North});

					pairs.push_back({O::North, O::Up});
					pairs.push_back({O::South, O::Up});
					pairs.push_back({O::East, O::Up});
					pairs.push_back({O::West, O::Up});
					break;
				}

			case Block::RotationType::Horizontal:
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
		std::vector<Block::Orientation> orientations = {Block::Orientation::Up,
														Block::Orientation::Down,
														Block::Orientation::North,
														Block::Orientation::South,
														Block::Orientation::East,
														Block::Orientation::West};

		// ---- Build valid (Facing, Top) pairs ----

		std::vector<OrientationPair> orientationPairs;

		auto IsPerpendicular = [](Block::Orientation a, Block::Orientation b)
		{
			if (a == b)
				return false;

			auto opposite = [](Block::Orientation o)
			{
				switch (o)
				{
					case Block::Orientation::Up:
						return Block::Orientation::Down;
					case Block::Orientation::Down:
						return Block::Orientation::Up;
					case Block::Orientation::North:
						return Block::Orientation::South;
					case Block::Orientation::South:
						return Block::Orientation::North;
					case Block::Orientation::East:
						return Block::Orientation::West;
					case Block::Orientation::West:
						return Block::Orientation::East;
					default:
						return Block::Orientation::None;
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

		const int max = WorldConstants::SUBCHUNK_SIZE - 1;

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

				Block block{id};
				block.m_Facing = pair.facing;
				block.m_Top = pair.top;

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

		int y = 0;

		// Bedrock layer
		Block bedrock{BlockId::Bedrock};
		for (y = 0; y <= 0; y++)
		{

			for (int x = 0; x < WorldConstants::SUBCHUNK_SIZE; x++)
			{
				for (int z = 0; z < WorldConstants::SUBCHUNK_SIZE; z++)
				{
					chunk->SetBlock(glm::ivec3(x, y, z), bedrock);
				}
			}
		}

		// 3 Dirt layers
		Block dirt{BlockId::Dirt};
		for (y = 1; y <= 3; y++)
		{
			for (int x = 0; x < WorldConstants::SUBCHUNK_SIZE; x++)
			{
				for (int z = 0; z < WorldConstants::SUBCHUNK_SIZE; z++)
				{
					chunk->SetBlock(glm::ivec3(x, y, z), dirt);
				}
			}
		}

		// Grass layer
		Block grass{BlockId::Grass};
		for (y = 4; y <= 4; y++)
		{
			for (int x = 0; x < WorldConstants::SUBCHUNK_SIZE; x++)
			{
				for (int z = 0; z < WorldConstants::SUBCHUNK_SIZE; z++)
				{
					chunk->SetBlock(glm::ivec3(x, y, z), grass);
				}
			}
		}

		return genChunk;
	}

	WorldGenerator::GenChunk WorldGenerator::GenerateChunk_Classic(const glm::ivec2& chunkPosition)
	{

		GenChunk genChunk;

		// Creates the Chunk
		auto chunk = std::make_shared<Chunk>(chunkPosition);

		constexpr int CHUNK_SIZE = WorldConstants::SUBCHUNK_SIZE;

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

				//// Random float number in range [-1, 1]
				//static std::mt19937 rng(std::random_device{}());
				//static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
				//heightMap[x][z] = static_cast<uint16_t>(GetTerrainHeight(dist(rng)));

				heightMap[x][z] =
					static_cast<uint16_t>(GetTerrainHeight(m_FastNoiseLite.GetNoise(noisePositionX, noisePositionZ)));
			}
		}

		// Fills the chunks with blocks based on the height map
		for (uint8_t z = 0; z < CHUNK_SIZE; ++z)
		{
			for (uint8_t x = 0; x < CHUNK_SIZE; ++x)
			{
				int realWorldX = (chunkPosition.x * CHUNK_SIZE + x);
				int realWorldZ = (chunkPosition.y * CHUNK_SIZE + z);
				uint16_t height = heightMap[x][z];

				// Higher than sea level
				if (height >= m_AverageHeight)
				{
					for (uint16_t y = 0; y < m_WorldHeight; ++y)
					{
						if (y == 0)
						{
							chunk->SetBlock({x, y, z}, BlockId::Bedrock); // Set bedrock at the bottom
						}
						else if (y == height)
						{
							chunk->SetBlock({x, y, z}, BlockId::Grass); // Set grass block at the top
						}
						else if (y == height - 1 || y == height - 2)
						{
							chunk->SetBlock({x, y, z}, BlockId::Dirt); // Set dirt blocks below the grass
						}
						else if (y < height)
						{
							chunk->SetBlock({x, y, z}, BlockId::Stone); // Set stone below the grass
						}
					}
				}

				// Lower than sea level
				if (height < m_AverageHeight)
				{
					for (uint16_t y = 0; y < m_WorldHeight; ++y)
					{
						if (y == 0)
						{
							chunk->SetBlock({x, y, z}, BlockId::Bedrock); // Set bedrock at the bottom
						}
						else if (y == height || y == height - 1)
						{
							chunk->SetBlock({x, y, z}, BlockId::Gravel); // Set gravel blocks below the sea level
						}
						else if (y < height)
						{
							chunk->SetBlock({x, y, z}, BlockId::Stone); // Set stone below the gravel
						}
						else if (y > height && y < m_AverageHeight)
						{
							chunk->SetBlock({x, y, z}, BlockId::Water); // Set water above the gravel
						}
					}
				}
			}
		}

		// Adds trees
		//for (uint8_t x = 0; x < World::CHUNK_SIZE_X; ++x)
		//{
		//	for (uint8_t z = 0; z < World::CHUNK_SIZE_Z; ++z)
		//	{
		//		int realWorldX = (position.x * World::CHUNK_SIZE_X + x);
		//		int realWorldZ = (position.z * World::CHUNK_SIZE_Z + z);
		//		int height = heightMap[x][z];

		//		if (height < m_AverageHeight)
		//			continue; // Skip if the height is below average (no trees in water)

		//		int minRand = 0;
		//		int maxRand = 1000000;

		//		// Seed PRNG directly
		//		std::uint64_t hash = 14695981039346656037ull; // FNV offset
		//		hash ^= static_cast<std::uint64_t>(m_Seed);
		//		hash *= 1099511628211ull;
		//		hash ^= static_cast<std::uint64_t>(realWorldX);
		//		hash *= 1099511628211ull;
		//		hash ^= static_cast<std::uint64_t>(realWorldZ);
		//		hash *= 1099511628211ull;

		//		std::mt19937 generator(static_cast<std::mt19937::result_type>(hash));
		//		std::uniform_int_distribution<int> heightDistribution(minRand, maxRand);

		//		// Generate a float in [0,1)
		//		float randomVal = static_cast<float>(heightDistribution(generator)) / (maxRand + 1);

		//		// 5% chance to generate a tree
		//		if (randomVal < 0.01f)
		//		{
		//			// Every 0 0
		//			// if (x == 0 && z == 0 && position.x == 0 && position.z == 0) {
		//			BlockPosition topBlock{
		//				(position.x * CHUNK_SIZE_X) + x, height - 1, (position.z * CHUNK_SIZE_Z) + z};
		//			Schematic tree = GenerateTree(topBlock); // Generate a tree at the top block position

		//			// Fuse the tree schematic into the chunk pile
		//			for (int treeX = 0; treeX < tree.SizeX; treeX++)
		//			{
		//				for (int treeY = 0; treeY < tree.SizeY; treeY++)
		//				{
		//					for (int treeZ = 0; treeZ < tree.SizeZ; treeZ++)
		//					{

		//						BlockState blockState = tree.GetBlockStateAt(treeX, treeY, treeZ);

		//						if (blockState.Id == BlockId::Air)
		//						{
		//							continue; // Skip air blocks
		//						}

		//						// Check if the block is in the chunkpile bounds
		//						const auto chunkpilePos =
		//							GetChunkPosition(tree.Origin.x + treeX, tree.Origin.z + treeZ);
		//						if (chunkpilePos == position)
		//						{
		//							// Set the block in the chunk pile

		//							// Get local coordinates in the chunk
		//							int localX = PositiveModulo((tree.Origin.x + treeX), World::CHUNK_SIZE_X);
		//							int localZ = PositiveModulo((tree.Origin.z + treeZ), World::CHUNK_SIZE_Z);
		//							chunk->SetBlock(localX, tree.Origin.y + treeY, localZ, blockState);
		//						}
		//						else
		//						{
		//							// Tree block is outside the chunk pile bounds ...
		//							int treeRealWorldX = tree.Origin.x + treeX;
		//							int treeRealWorldZ = tree.Origin.z + treeZ;

		//							// Add the block to the out-of-bounds blocks
		//							Block outOfBoundsBlock{
		//								treeRealWorldX, tree.Origin.y + treeY, treeRealWorldZ, blockState};
		//							genChunk.OutOfBoundsBlocks[chunkpilePos].push_back(outOfBoundsBlock);
		//						}
		//					}
		//				}
		//			}
		//		}
		//	}
		//}

		// Optimize the chunk (less memory, faster to send to clients)
		chunk->Optimize();

		genChunk.chunk = chunk;

		return genChunk;
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
