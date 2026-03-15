#pragma once

#include <FastNoiseLite.h>

#include <memory>
#include <random>
#include <thread>
#include <unordered_set>

#include <onion/ThreadPool.hpp>

#include <shared/world/World.hpp>
#include <shared/world/schematic/Schematic.hpp>
#include <shared/world/world_manager/WorldManager.hpp>

#include "SeededRandom.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace onion::voxel
{
	class WorldGenerator
	{
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

		// ----- Constructor / Destructor -----
	  public:
		WorldGenerator(std::shared_ptr<WorldManager> worldManager);
		~WorldGenerator();

		// ----- Public API -----
	  public:
		void GenerateChunkAsync(const glm::ivec2& chunkPosition);
		void GenerateChunksAsync(const std::vector<glm::ivec2>& chunkPositions);

		// ----- Getters / Setters -----
	  public:
		uint32_t GetSeed() const;

		// ----- Private Members -----
	  private:
		std::shared_ptr<WorldManager> m_WorldManager;

		// ----- Events Handlers -----
	  private:
		void SubscribeToWorldManagerEvents();
		std::vector<EventHandle> m_WorldManagerEventHandles;

		void Handle_SeedChanged(const uint32_t& newSeed);

		// ----- World Generation Settings -----
	  private:
		enum class eWorldGenerationType : uint8_t
		{
			DemoBlocks,
			Superflat,
			Classic,
			BiomeVisualizer,
		};

		eWorldGenerationType m_WorldGenerationType = eWorldGenerationType::Classic;

		// ----- Chunk Generation Thread -----
	  private:
		ThreadPool m_ThreadPool{4};

		GenChunk GenerateChunk(const glm::ivec2& chunkPosition);

		GenChunk GenerateChunk_DemoBlocks(const glm::ivec2& chunkPosition);
		GenChunk GenerateChunk_Superflat(const glm::ivec2& chunkPosition);
		GenChunk GenerateChunk_Classic(const glm::ivec2& chunkPosition);
		GenChunk GenerateChunk_BiomeVisualizer(const glm::ivec2& chunkPosition);

		// ----- Structures Generation -----
	  private:
		bool ShouldGenerateTree(const glm::ivec3& position) const;
		bool ShouldGenerateShortGrass(const glm::ivec3& position) const;
		bool ShouldGenerateFlower(const glm::ivec3& position) const;
		BlockId GetFlowerType(const glm::ivec3& position) const;

		BiomeBlend GetBiome(const glm::ivec3& pos) const;

		static Biome GetSeedBiome(int cellX, int cellZ);
		static BiomeBlend GetBiomeBlend(float x, float z);
		static float GetBiomeHeight(Biome biome);

		static std::vector<float> s_BiomeHeightsLookup;

		// ------------ STRUCTURES ------------
		static void MergeSchematicInChunk(const Schematic& schematic,
										  GenChunk& chunk,
										  const std::unordered_set<BlockId>& overwritables = {BlockId::Air});

		Schematic GenerateTree(const glm::ivec3& position, int height, BlockId logType, BlockId leavesType) const;

		// ----- Classic Generation Parameters -----
	  private:
		SeededRandom m_SeededRandom;

		void ConfigureNoiseGenerators();

		FastNoiseLite::NoiseType m_NoiseType = FastNoiseLite::NoiseType_OpenSimplex2;

		FastNoiseLite m_NoiseContinent;
		float m_FrequencyContinent = 0.0006f; // Smaller = smoother, larger = more rugged

		FastNoiseLite m_NoiseMountain;
		float m_FrequencyMountain = 0.003f; // Smaller = smoother, larger = more rugged

		FastNoiseLite m_NoiseDetail;
		float m_FrequencyDetail = 0.02f; // Smaller = smoother, larger = more rugged

		FastNoiseLite m_NoiseTemperature;
		float m_FrequencyTemperature = 0.0006f; // Smaller = smoother, larger = more rugged

		FastNoiseLite m_NoiseHumidity;
		float m_FrequencyHumidity = 0.0006f; // Smaller = smoother, larger = more rugged

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
