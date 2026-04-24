#include "WorldGenerator.hpp"

#include <iostream>

#include <shared/utils/Stopwatch.hpp>
#include <shared/utils/Utils.hpp>

#include <shared/world/block/BlockstateRegistry.hpp>

namespace
{
	constexpr int BIOME_CELL_SIZE = 1024; // blocks

	static inline uint32_t Hash(int x, int z)
	{
		uint32_t h = x * 374761393u + z * 668265263u;
		h = (h ^ (h >> 13)) * 1274126177u;
		return h ^ (h >> 16);
	}

	static inline float HashFloat(uint32_t h)
	{
		return (h & 0xFFFFFF) / float(0xFFFFFF);
	}

	static inline glm::vec2 GetSeedPosition(int cellX, int cellZ)
	{
		uint32_t h = Hash(cellX, cellZ);

		float rx = HashFloat(h);
		float rz = HashFloat(h * 48271u);

		return {cellX * BIOME_CELL_SIZE + rx * BIOME_CELL_SIZE, cellZ * BIOME_CELL_SIZE + rz * BIOME_CELL_SIZE};
	}

} // namespace

namespace onion::voxel
{
	// ----- Static Members Initialization -----

	const std::unordered_map<std::string, WorldGenerator::eWorldGenerationType>
		WorldGenerator::s_StringToWorldGenerationType = {
			{"Demo Blocks", eWorldGenerationType::DemoBlocks},
			{"Superflat", eWorldGenerationType::Superflat},
			{"Classic No Biomes", eWorldGenerationType::ClassicNoBiomes},
			{"Classic", eWorldGenerationType::Classic},
			{"BiomeVisualizer", eWorldGenerationType::BiomeVisualizer},
	};

	const std::unordered_map<WorldGenerator::eWorldGenerationType, std::string>
		WorldGenerator::s_WorldGenerationTypeToString = []()
	{
		std::unordered_map<eWorldGenerationType, std::string> m;
		for (const auto& pair : s_StringToWorldGenerationType)
		{
			m[pair.second] = pair.first;
		}
		return m;
	}();

	std::vector<float> WorldGenerator::s_BiomeHeightsLookup = []()
	{
		std::vector<float> lookup(static_cast<size_t>(Biome::Count));
		lookup[static_cast<size_t>(Biome::Ocean)] = 10.f;
		lookup[static_cast<size_t>(Biome::Plains)] = 75.f;
		lookup[static_cast<size_t>(Biome::Forest)] = 80.f;
		lookup[static_cast<size_t>(Biome::Desert)] = 70.f;
		lookup[static_cast<size_t>(Biome::Mountain)] = 150.f;
		lookup[static_cast<size_t>(Biome::Snow)] = 90.f;
		return lookup;
	}();

	std::vector<WorldGenerator::ColumnBlocks> WorldGenerator::s_BiomeColumnBlocksLookup = []()
	{
		std::vector<ColumnBlocks> lookup(static_cast<size_t>(Biome::Count));
		// Ocean
		lookup[static_cast<size_t>(Biome::Ocean)].layers = {{10, BlockId::Water}, {1, BlockId::Sand}};
		lookup[static_cast<size_t>(Biome::Ocean)].fillBlock = BlockId::Stone;

		// Plains
		lookup[static_cast<size_t>(Biome::Plains)].layers = {{1, BlockId::GrassBlock}, {2, BlockId::Dirt}};
		lookup[static_cast<size_t>(Biome::Plains)].fillBlock = BlockId::Stone;

		// Forest
		lookup[static_cast<size_t>(Biome::Forest)].layers = {{1, BlockId::GrassBlock}, {2, BlockId::Dirt}};
		lookup[static_cast<size_t>(Biome::Forest)].fillBlock = BlockId::Stone;

		// Desert
		lookup[static_cast<size_t>(Biome::Desert)].layers = {{3, BlockId::Sand}, {3, BlockId::Sandstone}};
		lookup[static_cast<size_t>(Biome::Desert)].fillBlock = BlockId::Stone;

		// Mountain
		lookup[static_cast<size_t>(Biome::Mountain)].layers = {{1, BlockId::GrassBlock}};
		lookup[static_cast<size_t>(Biome::Mountain)].fillBlock = BlockId::Stone;

		// Snow
		lookup[static_cast<size_t>(Biome::Snow)].layers = {{1, BlockId::SnowBlock}, {2, BlockId::Dirt}};
		lookup[static_cast<size_t>(Biome::Snow)].fillBlock = BlockId::Stone;
		return lookup;
	}();

	// ----- Constructor / Destructor -----

	WorldGenerator::WorldGenerator()
	{
		ConfigureNoiseGenerators();
	}

	WorldGenerator::~WorldGenerator()
	{
		std::cout << "~WorldGenerator" << std::endl;
		m_ThreadPool.Close();
	}

	void WorldGenerator::GenerateChunkAsync(const glm::ivec2& chunkPosition)
	{
		m_ThreadPool.Dispatch(
			[this, chunkPosition]()
			{
				GenChunk genChunk = GenerateChunk(chunkPosition);
				EvtChunkGenerated.Trigger(genChunk);
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
		return m_Seed;
	}

	void WorldGenerator::SetSeed(uint32_t seed)
	{
		m_Seed = seed;
		ConfigureNoiseGenerators();
	}

	WorldGenerator::eWorldGenerationType WorldGenerator::GetWorldGenerationType() const
	{
		return m_WorldGenerationType;
	}

	void WorldGenerator::SetWorldGenerationType(eWorldGenerationType worldGenerationType)
	{
		m_WorldGenerationType = worldGenerationType;
	}

	std::string WorldGenerator::WorldGenerationTypeToString(eWorldGenerationType type)
	{
		auto it = s_WorldGenerationTypeToString.find(type);
		if (it != s_WorldGenerationTypeToString.end())
		{
			return it->second;
		}
		else
		{
			return "Unknown";
		}
	}

	WorldGenerator::eWorldGenerationType WorldGenerator::StringToWorldGenerationType(const std::string& str)
	{
		auto it = s_StringToWorldGenerationType.find(str);
		if (it != s_StringToWorldGenerationType.end())
		{
			return it->second;
		}
		else
		{
			throw std::runtime_error("Invalid world generation type string: " + str);
		}
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
			case eWorldGenerationType::ClassicNoBiomes:
				genChunk = GenerateChunk_ClassicNoBiomes(chunkPosition);
				break;
			default:
				throw std::runtime_error("Invalid world generation type");
		}

		double elapsedMs = stopwatch.ElapsedMs();
		(void) elapsedMs; // Currently unused
		//std::cout << elapsedMs << " ms" << std::endl;

		return genChunk;
	}

	WorldGenerator::GenChunk WorldGenerator::GenerateChunk_DemoBlocks(const glm::ivec2& chunkPosition)
	{
		GenChunk genChunk;
		genChunk.chunk = std::make_shared<Chunk>(chunkPosition);
		auto& chunk = genChunk.chunk;

		constexpr int spacing = 3; // 1 block + 2 air gap
		constexpr int start = 1;   // avoid borders

		int x = start;
		int z = start;
		int y = start;

		const int max = WorldConstants::CHUNK_SIZE - 1;

		const auto& blockstates = BlockstateRegistry::Get();

		const int blockIdCount = BlockIds::GetBlockIdCount();
		for (int blockId = 0; blockId < blockIdCount; blockId++)
		{
			BlockId id = static_cast<BlockId>(blockId);

			if (id == BlockId::Air)
				continue;

			if (!blockstates.contains(id))
			{
				id = BlockId::PurpleConcrete;
			}

			const auto& variants = blockstates.at(id);
			uint8_t variantCount = static_cast<uint8_t>(variants.size());

			for (uint8_t variantIndex = 0; variantIndex < variantCount; variantIndex++)
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

				//if (y >= 1000)
				//	return genChunk;

				BlockState block(id);
				block.VariantIndex = variantIndex;

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
		constexpr bool GENERATE_TREES = true;

		int y = 0;

		// Bedrock layer
		BlockState bedrock(BlockId::Bedrock);
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
		BlockState dirt(BlockId::Dirt);
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
		BlockState grass(BlockId::GrassBlock);
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

					bool isTallFlower = BlockstateRegistry::IsTallPlant(flowerId);

					if (isTallFlower)
					{
						uint8_t lowerVariantIndex = BlockstateRegistry::GetVariantIndex(flowerId, {{"half", "lower"}});
						BlockState lowerFlower = BlockState(flowerId, lowerVariantIndex);
						chunk->SetBlock(glm::ivec3(x, height + 1, z), lowerFlower);

						uint8_t upperVariantIndex = BlockstateRegistry::GetVariantIndex(flowerId, {{"half", "upper"}});
						BlockState upperFlower = BlockState(flowerId, upperVariantIndex);
						chunk->SetBlock(glm::ivec3(x, height + 2, z), upperFlower);
					}
					else
					{
						chunk->SetBlock(glm::ivec3(x, height + 1, z), BlockState(flowerId));
					}
				}

				bool shouldGenerateTree = ShouldGenerateTree(worldPos);
				if (GENERATE_TREES && shouldGenerateTree)
				{
					double val = m_SeededRandom.GetValue(worldPos + glm::ivec3(25, 2584, 88));

					BlockId logId = (val < 0.1) ? BlockId::BirchLog : BlockId::OakLog; // 10% chance for birch
					BlockId leavesId = (logId == BlockId::BirchLog) ? BlockId::BirchLeaves : BlockId::OakLeaves;

					// Tree height between 2 and 6
					int treeHeight = 2 + (int) (m_SeededRandom.GetValue(worldPos + glm::ivec3(88, 654, 2584)) * 4);

					glm::ivec3 treeBlock = worldPos + glm::ivec3(0, 1, 0);
					Schematic tree = GenerateTree(
						treeBlock, treeHeight, logId, leavesId); // Generate a tree at the top block position

					std::unordered_set<BlockId> overwritables;
					overwritables.insert(BlockId::Air);
					overwritables.insert(BlockId::OakLeaves);
					overwritables.insert(BlockId::SpruceLeaves);
					overwritables.insert(BlockId::BirchLeaves);
					overwritables.insert(BlockId::Bush);
					overwritables.insert(BlockId::ShortGrass);
					overwritables.insert(BlockId::TallGrass);
					for (auto& blockId : BlockState::Flowers)
					{
						overwritables.insert(blockId);
					}

					MergeSchematicInChunk(tree, genChunk, overwritables);
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
				// ------ GENERATE HEIGHT MAP ------

				int realWorldX = (chunkPosition.x * CHUNK_SIZE + x);
				int realWorldZ = (chunkPosition.y * CHUNK_SIZE + z);

				float continents =
					GetFractalNoise(m_NoiseContinent, (float) realWorldX, (float) realWorldZ, 3, 2.0f, 0.5f);

				float mountains = std::max(
					0.0f, GetFractalNoise(m_NoiseMountain, (float) realWorldX, (float) realWorldZ, 4, 2.0f, 0.5f));

				float detail = GetFractalNoise(m_NoiseDetail, (float) realWorldX, (float) realWorldZ, 3, 2.0f, 0.5f);

				float continentMask = (continents + 1.0f) * 0.5f; // [-1,1] -> [0,1]
				continentMask = std::pow(continentMask, 1.5f);

				float noiseHeight = continents * 50.f + (mountains * continentMask) * 75.0f + detail * 1.0f;

				BiomeBlend biomeBlend = GetBiome(glm::ivec3(realWorldX, 0, realWorldZ));

				float height = 0;
				for (int i = 0; i < 9; i++)
				{
					const float h = GetBiomeHeight(biomeBlend.seeds[i].biome);
					height += biomeBlend.weights[i] * h;
				}

				height += noiseHeight;

				heightMap[x][z] =
					static_cast<uint16_t>(std::clamp(height, 1.0f, static_cast<float>(m_WorldHeight - 1)));
			}
		}

		// Fills the chunks with blocks based on the height map
		for (uint8_t z = 0; z < CHUNK_SIZE; z++)
		{
			for (uint8_t x = 0; x < CHUNK_SIZE; x++)
			{
				uint16_t height = heightMap[x][z];

				const glm::ivec3 worldPos = {
					chunkPosition.x * CHUNK_SIZE + x, height, chunkPosition.y * CHUNK_SIZE + z};
				const auto biomeBlend = GetBiome(worldPos);

				const auto& biome = biomeBlend.seeds[0].biome; // Get the dominant biome
				const ColumnBlocks& columnBlocks = GetColumnBlocksForBiome(biome);

				if (biome == Biome::Ocean)
				{
					ColumnBlocks oceanColumn;
					int waterDepth = m_SeaLevel - height;
					oceanColumn.layers.push_back({waterDepth, BlockId::Water});
					int gravelDepth = 5;
					oceanColumn.layers.push_back({gravelDepth, BlockId::Gravel});
					oceanColumn.fillBlock = BlockId::Stone;

					SetBlocksColumn(genChunk, glm::ivec2(x, z), m_SeaLevel, oceanColumn);

					// Fills from the height to sea level with gravel
					if (height > m_SeaLevel)
					{
						for (int y = m_SeaLevel; y <= height; y++)
						{
							chunk->SetBlock(glm::ivec3(x, y, z), BlockState(BlockId::Gravel));
						}
					}

					continue;
				}

				SetBlocksColumn(genChunk, glm::ivec2(x, z), height, columnBlocks);

				// Replace air blocks below sea level with water, and replace grass with dirt if underwater
				if (height <= m_SeaLevel)
				{
					for (uint16_t y = 0; y <= m_SeaLevel; y++)
					{
						BlockState block = chunk->GetBlock(glm::ivec3(x, y, z));
						if (block.ID == BlockId::Air)
						{
							chunk->SetBlock(glm::ivec3(x, y, z), BlockState(BlockId::Water));
							block.ID = BlockId::Water; // Update block variable for the next condition
						}
						else if ((block.ID == BlockId::GrassBlock || block.ID == BlockId::SnowBlock) && y < m_SeaLevel)
						{
							chunk->SetBlock(glm::ivec3(x, y, z), BlockState(BlockId::Dirt));
							block.ID = BlockId::Dirt; // Update block variable for the next condition
						}

						// Top Layer Replacement
						if (y == m_SeaLevel)
						{
							// Replace top water by ice in snowy biomes
							if (biome == Biome::Snow && block.ID == BlockId::Water)
							{
								chunk->SetBlock(glm::ivec3(x, y, z), BlockState(BlockId::Ice));
								block.ID = BlockId::Ice; // Update block variable for the next condition
							}
						}
					}
				}

				// If top block is not transparent, try to generate foliage on top of it
				bool isAboveSeaLevel = height > m_SeaLevel;
				bool isBlockTransparent = BlockState::IsTransparent(chunk->GetBlock(glm::ivec3(x, height, z)).ID);
				if (isAboveSeaLevel && !isBlockTransparent)
				{
					glm::ivec3 localPosAbove = {x, height + 1, z};
					AddFoliage(genChunk, worldPos + glm::ivec3(0, 1, 0), localPosAbove, biome);
				}
			}
		}

		// Optimize the chunk (less memory, faster to send to clients)
		chunk->Optimize();

		return genChunk;
	}

	WorldGenerator::GenChunk WorldGenerator::GenerateChunk_ClassicNoBiomes(const glm::ivec2& chunkPosition)
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
				// ------- GENERATE HEIGHT MAP WITHOUT BIOMES -------

				int realWorldX = (chunkPosition.x * CHUNK_SIZE + x);
				int realWorldZ = (chunkPosition.y * CHUNK_SIZE + z);

				float warpX =
					GetFractalNoise(m_NoiseWarp, (float) realWorldX, (float) realWorldZ, 2, 2.0f, 0.5f) * 200.0f;
				float warpZ =
					GetFractalNoise(m_NoiseWarp2, (float) realWorldX, (float) realWorldZ, 2, 2.0f, 0.5f) * 200.0f;

				float continents = GetFractalNoise(
					m_NoiseContinent, (float) realWorldX + warpX, (float) realWorldZ + warpZ, 3, 2.0f, 0.5f);

				float mountains =
					GetFractalNoise(m_NoiseMountain, (float) realWorldX, (float) realWorldZ, 4, 2.0f, 0.5f);
				float detail = GetFractalNoise(m_NoiseDetail, (float) realWorldX, (float) realWorldZ, 3, 2.0f, 0.5f);

				// Map continent noise to [0,1]
				float continentMask = (continents + 1.0f) * 0.5f;

				float mountainStart = 0.35f;
				float mountainEnd = 0.85f;

				// Create a mask for mountains so that they only appear in the higher parts of the continents
				continentMask = std::clamp((continentMask - mountainStart) / (mountainEnd - mountainStart), 0.0f, 1.0f);
				continentMask = continentMask * continentMask * (3.0f - 2.0f * continentMask);

				// Map mountains noise to [0,1], and apply a curve to have more flat areas and less extreme mountains
				mountains = (mountains + 1.0f) * 0.5f;
				mountains = mountains * mountains;

				// Apply the continent mask to the mountains so that they only appear on continents
				mountains *= continentMask;

				float height = m_SeaLevel + continents * 50.0f + mountains * 300.0f + detail * 5.0f;

				heightMap[z][x] =
					static_cast<uint16_t>(std::clamp(height, 1.0f, static_cast<float>(m_WorldHeight - 1)));
			}
		}

		// Fills the chunks with blocks based on the height map
		for (uint8_t z = 0; z < CHUNK_SIZE; z++)
		{
			for (uint8_t x = 0; x < CHUNK_SIZE; x++)
			{
				uint16_t height = heightMap[z][x];

				const glm::ivec3 worldPos = {
					chunkPosition.x * CHUNK_SIZE + x, height, chunkPosition.y * CHUNK_SIZE + z};

				int adjustedSnowLevel = m_SnowLevel;
				double val = m_SeededRandom.GetValue(worldPos);
				constexpr int snowLevelVariation = 10;

				// From range [0,1] to range [-snowLevelVariation, snowLevelVariation]
				int snowLevelOffset = static_cast<int>((val * 2.0 - 1.0) * snowLevelVariation);
				adjustedSnowLevel += snowLevelOffset;

				// Higher than sea level
				if (height >= m_SeaLevel)
				{
					BlockId topBlockId = (height >= adjustedSnowLevel) ? BlockId::SnowBlock : BlockId::GrassBlock;
					for (uint16_t y = 0; y <= height + 1; y++)
					{
						// Bedrock
						if (y == 0)
						{
							chunk->SetBlock_Unsafe(
								(uint8_t) x, y, (uint8_t) z, BlockState(BlockId::Bedrock)); // Set bedrock at the bottom
							continue;
						}

						float altitudeFactor = std::clamp(
							(float) (height - m_SeaLevel) / (float) (adjustedSnowLevel - m_SeaLevel), 0.0f, 1.0f);
						int numDirtLayers = (int) std::round((1.0f - altitudeFactor) * 3.0f);

						// Stone layers
						if (y < height - numDirtLayers)
						{
							chunk->SetBlock_Unsafe(
								(uint8_t) x, y, (uint8_t) z, BlockState(BlockId::Stone)); // Set stone below the dirt
							continue;
						}

						// Dirt layers
						if (y < height)
						{
							chunk->SetBlock_Unsafe(
								(uint8_t) x, y, (uint8_t) z, BlockState(BlockId::Dirt)); // Set dirt below the top block
							continue;
						}

						if (y == height)
						{
							chunk->SetBlock_Unsafe((uint8_t) x,
												   y,
												   (uint8_t) z,
												   BlockState(topBlockId)); // Set the Top block (grass or snow grass)
							continue;
						}
					}
				}
				else
				{
					for (uint16_t y = 0; y < m_WorldHeight; y++)
					{
						// Bedrock
						if (y == 0)
						{
							chunk->SetBlock_Unsafe(
								(uint8_t) x, y, (uint8_t) z, BlockState(BlockId::Bedrock)); // Set bedrock at the bottom
						}
						// Stone
						else if (y < height - 3)
						{
							chunk->SetBlock_Unsafe(
								(uint8_t) x, y, (uint8_t) z, BlockState(BlockId::Stone)); // Set stone at the bottom
						}
						// Gravel Or Sand
						else if (y <= height)
						{
							// Deeper sea has gravel, shallower sea has sand
							BlockId seaFloor = (height < m_SeaLevel - 8) ? BlockId::Gravel : BlockId::Sand;
							chunk->SetBlock_Unsafe((uint8_t) x, y, (uint8_t) z, BlockState(seaFloor));
						}
						else if (y > height && y < m_SeaLevel)
						{
							chunk->SetBlock_Unsafe(
								(uint8_t) x, y, (uint8_t) z, BlockState(BlockId::Water)); // Set water above the gravel
						}
					}
				}

				// If top block is not transparent, try to generate foliage on top of it
				bool isAboveSeaLevel = height > m_SeaLevel;
				bool isBlockTransparent = BlockState::IsTransparent(chunk->GetBlock(glm::ivec3(x, height, z)).ID);
				if (isAboveSeaLevel && !isBlockTransparent)
				{
					glm::ivec3 localPosAbove = {x, height + 1, z};
					AddFoliage(genChunk, worldPos + glm::ivec3(0, 1, 0), localPosAbove, Biome::Forest);
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
						blockId = BlockId::GrassBlock;
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
						blockId = BlockId::SnowBlock;
						break;
					case Biome::Ocean:
						blockId = BlockId::Water;
						break;

					default:
						blockId = BlockId::Dirt;
						break;
				}

				float height = 0;
				for (int i = 0; i < 9; i++)
				{
					const float h = GetBiomeHeight(biomeBlend.seeds[i].biome);
					height += biomeBlend.weights[i] * h;
				}

				for (int y = 0; y < height; y++)
				{
					chunk->SetBlock(glm::ivec3(x, y, z), BlockState(blockId));
				}
			}
		}

		return genChunk;
	}

	bool WorldGenerator::ShouldGenerateTree(const glm::ivec3& position, Biome biome) const
	{
		double randomVal = m_SeededRandom.GetValue(position + glm::ivec3(25, 2584, 88));

		// Less trees at higher altitudes
		const double altitudeFactor = static_cast<double>(position.y) / static_cast<double>(m_SnowLevel);

		//constexpr double BASE_TREE_CHANCE = 0.02; // Base 2% chance to generate a tree

		// Base Tree chance depends on the biome
		double baseTreeChance = 0.0;
		switch (biome)
		{
			case Biome::Plains:
				baseTreeChance = 0.003;
				break;
			case Biome::Forest:
				baseTreeChance = 0.02;
				break;
			case Biome::Desert:
				baseTreeChance = 0.0005;
				break;
			case Biome::Mountain:
				baseTreeChance = 0.003;
				break;
			case Biome::Snow:
				baseTreeChance = 0.003;
				break;
			default:
				break;
		}

		randomVal += altitudeFactor * baseTreeChance; // Increase the random value slightly based on altitude

		return randomVal < baseTreeChance;
	}

	bool WorldGenerator::ShouldGenerateShortGrass(const glm::ivec3& position, Biome biome) const
	{
		double randomVal = m_SeededRandom.GetValue(position + glm::ivec3(745, 324, 199));

		// Less short grass at higher altitudes
		double altitudeFactor = static_cast<double>(position.y) / static_cast<double>(m_SnowLevel);

		//constexpr double BASE_SHORT_GRASS_CHANCE = 0.25; // Base 25% chance to generate short grass

		// Get base chance depending on the biome
		double baseChance = 0.0;
		switch (biome)
		{
			case Biome::Plains:
				baseChance = 0.25;
				break;
			case Biome::Forest:
				baseChance = 0.05;
				break;
			case Biome::Desert:
				baseChance = 0.01;
				break;
			case Biome::Mountain:
				baseChance = 0.1;
				break;
			case Biome::Snow:
				baseChance = 0.05;
				break;
			default:
				break;
		}

		if (m_WorldGenerationType == eWorldGenerationType::ClassicNoBiomes)
		{
			baseChance = 0.25; // In ClassicNoBiomes, use the plains chance for short grass
		}

		randomVal += altitudeFactor * baseChance; // Increase the random value slightly based on altitude

		return randomVal < baseChance;
	}

	bool WorldGenerator::ShouldGenerateFlower(const glm::ivec3& position, Biome biome) const
	{
		double randomVal = m_SeededRandom.GetValue(position + glm::ivec3(1, 46, 789));

		// Less flowers at higher altitudes
		double altitudeFactor = static_cast<double>(position.y) / static_cast<double>(m_SnowLevel);

		//constexpr double BASE_FLOWER_CHANCE = 0.05; // Base 5% chance to generate flowers

		double baseFlowerChance = 0.0;
		switch (biome)
		{
			case Biome::Plains:
				baseFlowerChance = 0.05;
				break;
			case Biome::Forest:
				baseFlowerChance = 0.02;
				break;
			case Biome::Desert:
				baseFlowerChance = 0.001;
				break;
			case Biome::Mountain:
				baseFlowerChance = 0.01;
				break;
			case Biome::Snow:
				baseFlowerChance = 0.01;
				break;
			default:
				break;
		}

		if (m_WorldGenerationType == eWorldGenerationType::ClassicNoBiomes)
		{
			baseFlowerChance = 0.05; // In ClassicNoBiomes, use the plains chance for flowers
		}

		randomVal += altitudeFactor * baseFlowerChance; // Increase the random value slightly based on altitude
		return randomVal < baseFlowerChance;
	}

	BlockId WorldGenerator::GetFlowerType(const glm::ivec3& position) const
	{
		const double val = m_SeededRandom.GetValue(position);
		const double flowerVal = m_SeededRandom.GetValue(val);
		const size_t index = static_cast<size_t>(flowerVal * BlockState::Flowers.size());
		return BlockState::Flowers[index];
	}

	void WorldGenerator::SetBlocksColumn(GenChunk& genChunk,
										 const glm::ivec2& localPos,
										 int height,
										 const ColumnBlocks& columnBlocks)
	{
		const auto& chunk = genChunk.chunk;
		const auto& layers = columnBlocks.layers;

		int y = height;

		for (auto& [layerHeight, blockId] : layers)
		{
			for (int i = 0; i < layerHeight && y > 0; i++)
			{
				chunk->SetBlock(glm::ivec3(localPos.x, y, localPos.y), BlockState(blockId));
				y--;
			}
		}

		// Fill remaining height with the last block type
		while (y > 0)
		{
			chunk->SetBlock(glm::ivec3(localPos.x, y, localPos.y), BlockState(columnBlocks.fillBlock));
			y--;
		}

		// Ensure bedrock at the bottom
		chunk->SetBlock(glm::ivec3(localPos.x, 0, localPos.y), BlockState(BlockId::Bedrock));
	}

	const WorldGenerator::ColumnBlocks& WorldGenerator::GetColumnBlocksForBiome(Biome biome) const
	{
		return s_BiomeColumnBlocksLookup[static_cast<size_t>(biome)];
	}

	WorldGenerator::BiomeBlend WorldGenerator::GetBiome(const glm::ivec3& pos) const
	{
		const float noiseX = GetFractalNoise(m_NoiseWarp, (float) pos.x, (float) pos.z, 4, 2.0f, 0.5f) * 200.f;
		const float noiseZ = GetFractalNoise(m_NoiseWarp2, (float) pos.z, (float) pos.x, 4, 2.0f, 0.5f) * 200.f;

		const float x = pos.x + noiseX;
		const float z = pos.z + noiseZ;

		return GetBiomeBlend(x, z);
	}

	Biome WorldGenerator::GetSeedBiome(int cellX, int cellZ)
	{
		uint32_t h = Hash(cellX, cellZ);
		auto index = h % static_cast<uint32_t>(Biome::Count);
		return static_cast<Biome>(index);
	}

	WorldGenerator::BiomeBlend WorldGenerator::GetBiomeBlend(float x, float z)
	{
		const int cellX = static_cast<int>(std::floor(x / BIOME_CELL_SIZE));
		const int cellZ = static_cast<int>(std::floor(z / BIOME_CELL_SIZE));

		WorldGenerator::BiomeBlend blend;
		for (int i = 0; i < 9; i++)
		{
			blend.seeds[i] = {FLT_MAX, Biome::Plains};
			blend.weights[i] = 0.0f;
		}

		int index = 0;
		for (int dz = -1; dz <= 1; dz++)
		{
			for (int dx = -1; dx <= 1; dx++)
			{
				const int cx = cellX + dx;
				const int cz = cellZ + dz;

				const glm::vec2 seed = GetSeedPosition(cx, cz);

				const float dxs = seed.x - x;
				const float dzs = seed.y - z;

				const float dist = sqrt(dxs * dxs + dzs * dzs);

				const Biome biome = GetSeedBiome(cx, cz);

				blend.seeds[index] = {dist, biome};
				index++;
			}
		}

		// Sort by distance (closest first)
		std::sort(std::begin(blend.seeds),
				  std::end(blend.seeds),
				  [](const WorldGenerator::BiomeSeed& a, const WorldGenerator::BiomeSeed& b)
				  { return a.dist < b.dist; });

		// Lower k = smoother biome transitions, higher k = sharper biome transitions
		constexpr float K = 30.0f / BIOME_CELL_SIZE;

		float sum = 0;
		for (int i = 0; i < 9; i++)
		{
			blend.weights[i] = exp(-blend.seeds[i].dist * K);
			sum += blend.weights[i];
		}

		for (int i = 0; i < 9; i++)
		{
			blend.weights[i] /= sum;
		}

		return blend;
	}

	float WorldGenerator::GetBiomeHeight(Biome biome)
	{
		return s_BiomeHeightsLookup[static_cast<size_t>(biome)];
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

	Schematic WorldGenerator::GenerateTree(const glm::ivec3& position, int height, BlockId logType, BlockId leavesType)
	{
		glm::ivec3 treePosition{position.x, position.y, position.z};

		glm::ivec3 SchematicOrigin{treePosition.x - 2, treePosition.y, treePosition.z - 2};

		Schematic treeSchematic({5, 20, 5}, SchematicOrigin);

		constexpr int MIN_TREE_HEIGHT = 2;

		int treeHeight = std::max(height, MIN_TREE_HEIGHT);

		static std::random_device rd;  // Non-deterministic (hardware RNG)
		static std::mt19937 gen(rd()); // Only seeded once
		static std::uniform_int_distribution<int> dist(1, 100);

		// Creates the block palette for the tree
		// axis=y is variant index 1 in all Minecraft log blockstates (axis=x=0, axis=y=1, axis=z=2)
		BlockState verticalLog(logType);
		verticalLog.VariantIndex = 1;
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

	Schematic WorldGenerator::GenerateCactus(const glm::ivec3& position, int height)
	{
		glm::ivec3 cactusPosition{position.x, position.y, position.z};
		glm::ivec3 SchematicOrigin{cactusPosition.x, cactusPosition.y, cactusPosition.z};
		Schematic cactusSchematic({1, 20, 1}, SchematicOrigin);
		constexpr int MIN_CACTUS_HEIGHT = 2;
		int cactusHeight = std::max(height, MIN_CACTUS_HEIGHT);

		BlockState cactusBlock = BlockState(BlockId::Cactus);
		for (int y = 0; y < cactusHeight; y++)
		{
			cactusSchematic.SetBlock({0, y, 0}, cactusBlock); // Set the cactus block
		}

		// Flower on top of the cactus
		cactusSchematic.SetBlock({0, cactusHeight, 0}, BlockState(BlockId::CactusFlower));
		return cactusSchematic;
	}

	void WorldGenerator::AddFoliage(GenChunk& genChunk, const glm::ivec3& worldPos, glm::ivec3& localPos, Biome biome)
	{
		bool shouldGenerateTree = ShouldGenerateTree(worldPos, biome);

		if (shouldGenerateTree)
		{
			BlockId logId = BlockId::OakLog;
			BlockId leavesId = BlockId::OakLeaves;
			int minTreeHeight = 2;

			switch (biome)
			{
				case Biome::Snow:
					logId = BlockId::SpruceLog;
					leavesId = BlockId::SpruceLeaves;
					minTreeHeight = 6;
					break;
				case Biome::Plains:
					logId = BlockId::BirchLog;
					leavesId = BlockId::BirchLeaves;
					minTreeHeight = 4;
					break;
				default:
					break;
			}

			// Tree height between minTreeHeight and minTreeHeight + 4
			int treeHeight =
				minTreeHeight + static_cast<int>(m_SeededRandom.GetValue(worldPos + glm::ivec3(41, 588, 77)) * 4);

			Schematic tree(glm::ivec3(0));
			if (biome == Biome::Desert)
			{
				tree = GenerateCactus(worldPos, treeHeight);
			}
			else
			{
				tree = GenerateTree(worldPos, treeHeight, logId, leavesId);
			}

			std::unordered_set<BlockId> overwritables;
			overwritables.insert(BlockId::Air);
			overwritables.insert(BlockId::OakLeaves);
			overwritables.insert(BlockId::SpruceLeaves);
			overwritables.insert(BlockId::BirchLeaves);
			overwritables.insert(BlockId::Bush);
			overwritables.insert(BlockId::ShortGrass);
			overwritables.insert(BlockId::TallGrass);
			for (auto& blockId : BlockState::Flowers)
			{
				overwritables.insert(blockId);
			}
			MergeSchematicInChunk(tree, genChunk, overwritables);

			// Set dirt block under the tree
			glm::ivec3 belowTree = localPos + glm::ivec3(0, -1, 0);
			genChunk.chunk->SetBlock(belowTree, BlockState(BlockId::Dirt));
		}

		// If top block is available
		if (genChunk.chunk->GetBlock(localPos).ID == BlockId::Air)
		{
			if (ShouldGenerateShortGrass(worldPos, biome))
			{
				BlockId grassId = (biome == Biome::Desert) ? BlockId::DeadBush : BlockId::ShortGrass;
				genChunk.chunk->SetBlock(localPos, BlockState(grassId));
			}
			else if (ShouldGenerateFlower(worldPos, biome))
			{
				BlockId flowerId = GetFlowerType(worldPos + glm::ivec3(4520, 4541, 970));
				genChunk.chunk->SetBlock(localPos, BlockState(flowerId));
			}
		}

		// Chose block palette based on biome
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
