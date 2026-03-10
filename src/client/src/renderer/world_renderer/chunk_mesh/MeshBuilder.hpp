#pragma once

#include <glm/glm.hpp>

#include <chrono>
#include <deque>
#include <memory>

#include <onion/ThreadPool.hpp>

#include <shared/world/chunk/Chunk.hpp>
#include <shared/world/world_manager/WorldManager.hpp>

#include "../block_registry/BlockRegistry.hpp"
#include "ChunkMesh.hpp"

namespace onion::voxel
{
	class MeshBuilder
	{
		// ----- Constructor / Destructor -----
	  public:
		MeshBuilder(std::shared_ptr<WorldManager> worldManager, std::shared_ptr<TextureAtlas> textureAtlas);
		~MeshBuilder();

		// ----- Public API -----
	  public:
		void UpdateChunkMeshAsync(const std::shared_ptr<ChunkMesh> chunkMesh);
		double GetAverageChunkMeshUpdateTime() const; // Returns the average time taken for chunk mesh updates in ms
		size_t GetChunkMeshUpdatesLastSeconds() const;
		double GetChunkMeshUpdatesPerSecond() const;

		size_t GetMeshBuilderThreadCount() const;
		void SetMeshBuilderThreadCount(size_t count);

		// ----- Private Members -----
	  private:
		std::shared_ptr<WorldManager> m_WorldManager;
		BlockRegistry m_BlockRegistry;
		std::shared_ptr<TextureAtlas> m_TextureAtlas;

		ThreadPool m_ThreadPool{4};

		// ----- Latency Metrics -----
	  private:
		size_t m_MaxDurationsToStore = 100;			  // Maximum number of durations to store for averaging
		std::mutex m_ChunkMeshUpdateTimesMutex;		  // Mutex to protect access to the chunk mesh update times
		std::deque<double> m_ChunkMeshUpdateTimes_ms; // Stores the time taken for each chunk mesh update
		void AddChunkMeshUpdateTime(double timeMs);	  // Adds a new chunk mesh update time and updates the average
		std::atomic<double> m_AverageChunkMeshUpdateTime{0.0}; // The average time taken for chunk mesh updates

		// ----- Execution Frequency Metrics -----
	  private:
		std::chrono::seconds m_Window{1}; // window size (last X seconds)
		mutable std::mutex m_ExecutionTimesMutex;
		std::deque<std::chrono::steady_clock::time_point> m_ExecutionTimes;
		void RecordExecution();

		// ----- Private Methods -----
	  private:
		void UpdateChunkMesh(const std::shared_ptr<ChunkMesh> chunkMesh);

		static void AddFace(SubChunkMesh& mesh,
							const glm::ivec3& v0,
							const glm::ivec3& v1,
							const glm::ivec3& v2,
							const glm::ivec3& v3,
							BlockFace face,
							const Block& block,
							const FaceTexture& faceTexture,
							const TextureAtlas::AtlasEntry& uv,
							Block::RotationType rotationType);
	};
} // namespace onion::voxel
