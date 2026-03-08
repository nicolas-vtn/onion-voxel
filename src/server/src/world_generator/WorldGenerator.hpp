#pragma once

#include <memory>
#include <thread>

#include <shared/thread_safe_queue/ThreadSafeQueue.hpp>
#include <shared/world/world_manager/WorldManager.hpp>

namespace onion::voxel
{
	class WorldGenerator
	{
		// ----- Constructor / Destructor -----
	  public:
		WorldGenerator(std::shared_ptr<WorldManager> worldManager);
		~WorldGenerator();

		// ----- Public API -----
	  public:
		void GenerateChunkAsync(const glm::ivec2& chunkPosition);
		void GenerateChunksAsync(const std::vector<glm::ivec2>& chunkPositions);

		// ----- Private Members -----
	  private:
		std::shared_ptr<WorldManager> m_WorldManager;
		ThreadSafeQueue<glm::ivec2> m_ChunkGenerationQueue;

		// ----- World Generation Settings -----
	  private:
		enum class eWorldGenerationType : uint8_t
		{
			DemoBlocks,
			Superflat,
			Classic
		};

		eWorldGenerationType m_WorldGenerationType = eWorldGenerationType::DemoBlocks;

		// ----- Chunk Generation Thread -----
	  private:
		std::jthread m_ThreadChunkGeneration;
		void ChunkGenerationThreadFunction(std::stop_token st);
		std::shared_ptr<Chunk> GenerateChunk(const glm::ivec2& chunkPosition);

		std::shared_ptr<Chunk> GenerateChunk_DemoBlocks(const glm::ivec2& chunkPosition);
		std::shared_ptr<Chunk> GenerateChunk_Superflat(const glm::ivec2& chunkPosition);
		std::shared_ptr<Chunk> GenerateChunk_Classic(const glm::ivec2& chunkPosition);
	};
} // namespace onion::voxel
