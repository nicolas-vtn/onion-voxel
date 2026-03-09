#pragma once

#include <FastNoiseLite.h>

#include <memory>
#include <random>
#include <thread>

#include <onion/ThreadPool.hpp>

#include <shared/thread_safe_queue/ThreadSafeQueue.hpp>
#include <shared/world/schematic/Schematic.hpp>
#include <shared/world/world_manager/WorldManager.hpp>

namespace std
{
	template <> struct hash<glm::ivec3>
	{
		size_t operator()(const glm::ivec3& v) const noexcept
		{
			size_t h = 0;

			h ^= std::hash<int>()(v.x) + 0x9e3779b9 + (h << 6) + (h >> 2);
			h ^= std::hash<int>()(v.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
			h ^= std::hash<int>()(v.z) + 0x9e3779b9 + (h << 6) + (h >> 2);

			return h;
		}
	};
} // namespace std

namespace onion::voxel
{
	class WorldGenerator
	{

		// ----- Structs -----
	  public:
		struct GenChunk
		{
			std::shared_ptr<Chunk> chunk;
			std::unordered_map<glm::ivec3, Block> outOfBoundsBlocks;
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
			Classic
		};

		eWorldGenerationType m_WorldGenerationType = eWorldGenerationType::Classic;

		// ----- Chunk Generation Thread -----
	  private:
		ThreadPool m_ThreadPool{4};

		GenChunk GenerateChunk(const glm::ivec2& chunkPosition);

		GenChunk GenerateChunk_DemoBlocks(const glm::ivec2& chunkPosition);
		GenChunk GenerateChunk_Superflat(const glm::ivec2& chunkPosition);
		GenChunk GenerateChunk_Classic(const glm::ivec2& chunkPosition);

		// ----- Structures Generation -----
	  private:
		// ------------ STRUCTURES ------------
		Schematic GenerateTree(const glm::ivec3& position) const;

		// ----- Classic Generation Parameters -----
	  private:
		FastNoiseLite m_FastNoiseLite; // Noise generator for terrain generation
		FastNoiseLite::NoiseType m_NoiseType = FastNoiseLite::NoiseType_OpenSimplex2;
		float m_Frequency = 0.01f; // Frequency of the noise (controls "zoom" of the terrain)
		void ConfigureNoiseGenerator();

		uint16_t m_WorldHeight = 500;  // Height of the world in blocks
		float m_SmoothnessX = 0.5f;	   // Controls the "zoom" of the terrain in the X direction
		float m_SmoothnessZ = 0.5f;	   // Controls the "zoom" of the terrain in the Z direction
		float m_AverageHeight = 70.0f; // Average height of the terrain
		float m_Amplitude = 35.0f;	   // Amplitude of the noise

		constexpr float GetTerrainHeight(float noiseHeight) const;
	};
} // namespace onion::voxel
