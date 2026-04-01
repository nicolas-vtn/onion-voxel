#pragma once

#include <FastNoiseLite.h>

#include <memory>
#include <random>
#include <thread>
#include <unordered_set>

#include <onion/Event.hpp>
#include <onion/ThreadPool.hpp>

#include <shared/world/World.hpp>
#include <shared/world/chunk/Chunk.hpp>
#include <shared/world/schematic/Schematic.hpp>
//#include <shared/world/world_manager/WorldManager.hpp>

#include "SeededRandom.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace onion::voxel
{
	class WorldGenerator
	{
		// ----- World Generation Settings -----
	  public:
		enum class eWorldGenerationType : uint8_t
		{
			DemoBlocks = 0,
			Superflat,
			ClassicNoBiomes,
			Classic,
			BiomeVisualizer,

			Count,
		};

		// ----- Structs -----
	  public:
		struct GenChunk
		{
			std::shared_ptr<Chunk> chunk;
			std::vector<Block> outOfBoundsBlocks;
		};

	  private:
		struct BiomeSeed
		{
			float dist;
			Biome biome;
		};

		struct BiomeBlend
		{
			BiomeSeed seeds[9];
			float weights[9];
		};

		struct ColumnBlocks
		{
			// pair<height, blockId>, sorted by height descending (top block first)
			std::vector<std::pair<int, BlockId>> layers;
			BlockId fillBlock = BlockId::Stone;
		};

		// ----- Constructor / Destructor -----
	  public:
		WorldGenerator();
		~WorldGenerator();

		// ----- Public API -----
	  public:
		void GenerateChunkAsync(const glm::ivec2& chunkPosition);
		void GenerateChunksAsync(const std::vector<glm::ivec2>& chunkPositions);

		// ----- Getters / Setters -----
	  public:
		uint32_t GetSeed() const;
		void SetSeed(uint32_t seed);

		eWorldGenerationType GetWorldGenerationType() const;
		void SetWorldGenerationType(eWorldGenerationType worldGenerationType);

		// ----- Helpers -----
	  private:
		static const std::unordered_map<std::string, eWorldGenerationType> s_StringToWorldGenerationType;
		static const std::unordered_map<eWorldGenerationType, std::string> s_WorldGenerationTypeToString;

	  public:
		static std::string WorldGenerationTypeToString(eWorldGenerationType type);
		static eWorldGenerationType StringToWorldGenerationType(const std::string& str);

		// ----- Events -----
	  public:
		Event<const GenChunk&> EvtChunkGenerated;

		// ----- Private Members -----
	  private:
		eWorldGenerationType m_WorldGenerationType = eWorldGenerationType::Superflat;
		std::atomic_uint32_t m_Seed{1};

		// ----- Chunk Generation Thread -----
	  private:
		ThreadPool m_ThreadPool{4};

		GenChunk GenerateChunk(const glm::ivec2& chunkPosition);

		GenChunk GenerateChunk_DemoBlocks(const glm::ivec2& chunkPosition);
		GenChunk GenerateChunk_Superflat(const glm::ivec2& chunkPosition);
		GenChunk GenerateChunk_Classic(const glm::ivec2& chunkPosition);
		GenChunk GenerateChunk_ClassicNoBiomes(const glm::ivec2& chunkPosition);
		GenChunk GenerateChunk_BiomeVisualizer(const glm::ivec2& chunkPosition);

		// ----- Structures Generation -----
	  private:
		bool ShouldGenerateTree(const glm::ivec3& position, Biome biome = Biome::Plains) const;
		bool ShouldGenerateShortGrass(const glm::ivec3& position, Biome biome = Biome::Plains) const;
		bool ShouldGenerateFlower(const glm::ivec3& position, Biome biome = Biome::Plains) const;
		BlockId GetFlowerType(const glm::ivec3& position) const;

		void SetBlocksColumn(GenChunk& chunk, const glm::ivec2& localPos, int height, const ColumnBlocks& columnBlocks);
		const ColumnBlocks& GetColumnBlocksForBiome(Biome biome) const;

		BiomeBlend GetBiome(const glm::ivec3& pos) const;

		static Biome GetSeedBiome(int cellX, int cellZ);
		static BiomeBlend GetBiomeBlend(float x, float z);
		static float GetBiomeHeight(Biome biome);

		static std::vector<float> s_BiomeHeightsLookup;
		static std::vector<ColumnBlocks> s_BiomeColumnBlocksLookup;

		// ------------ STRUCTURES ------------
		static void MergeSchematicInChunk(const Schematic& schematic,
										  GenChunk& chunk,
										  const std::unordered_set<BlockId>& overwritables = {BlockId::Air});

		static Schematic GenerateTree(const glm::ivec3& position, int height, BlockId logType, BlockId leavesType);
		static Schematic GenerateCactus(const glm::ivec3& position, int height);

		void AddFoliage(GenChunk& genChunk, const glm::ivec3& worldPosition, glm::ivec3& localPosition, Biome biome);

		// ----- Classic Generation Parameters -----
	  private:
		SeededRandom m_SeededRandom;

		void ConfigureNoiseGenerators();

		FastNoiseLite::NoiseType m_NoiseType = FastNoiseLite::NoiseType_OpenSimplex2;

		FastNoiseLite m_NoiseContinent;
		float m_FrequencyContinent = 0.0006f; // Smaller = smoother, larger = more rugged

		FastNoiseLite m_NoiseMountain;
		float m_FrequencyMountain = 0.0015f; // Smaller = smoother, larger = more rugged

		FastNoiseLite m_NoiseDetail;
		float m_FrequencyDetail = 0.02f; // Smaller = smoother, larger = more rugged

		FastNoiseLite m_NoiseWarp;
		float m_FrequencyWarp = 0.0008f; // Smaller = smoother, larger = more rugged

		FastNoiseLite m_NoiseWarp2;
		float m_FrequencyWarp2 = 0.0008f; // Smaller = smoother, larger = more rugged

		float
		GetFractalNoise(const FastNoiseLite& noise, float x, float z, int octaves, float lacunarity, float gain) const;

		uint16_t m_WorldHeight = 500; // Height of the world in blocks
		float m_SmoothnessX = 0.5f;	  // Controls the "zoom" of the terrain in the X direction
		float m_SmoothnessZ = 0.5f;	  // Controls the "zoom" of the terrain in the Z direction
		int m_SeaLevel = 64;		  // Sea level of the terrain
		int m_SnowLevel = 200;		  // Snow level of the terrain (blocks above this height will be covered in snow)
		int m_MaxTerrainHeight = 300; // Maximum height of the terrain

		constexpr float GetTerrainHeight(float noiseHeight) const;
	};
} // namespace onion::voxel
