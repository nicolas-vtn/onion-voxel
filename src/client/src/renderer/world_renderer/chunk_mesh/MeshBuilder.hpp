#pragma once

#include <glm/glm.hpp>

#include <chrono>
#include <deque>
#include <memory>

#include <onion/ThreadPool.hpp>

#include <shared/world/chunk/Chunk.hpp>
#include <shared/world/world_manager/WorldManager.hpp>

#include <renderer/world_renderer/block_registry/BlockRegistry.hpp>

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
		void Initialize();

		void UpdateChunkMeshAsync(const std::shared_ptr<ChunkMesh> chunkMesh);
		double GetAverageChunkMeshUpdateTime() const; // Returns the average time taken for chunk mesh updates in ms
		size_t GetChunkMeshUpdatesLastSeconds() const;
		double GetChunkMeshUpdatesPerSecond() const;

		size_t GetMeshBuilderThreadCount() const;
		void SetMeshBuilderThreadCount(size_t count);

		const std::unordered_set<std::string>& GetAllRegisteredTextureNames() const;

		// ----- Private Members -----
	  private:
		std::shared_ptr<WorldManager> m_WorldManager;
		BlockRegistry m_BlockRegistry;
		std::shared_ptr<TextureAtlas> m_TextureAtlas;

		ThreadPool m_ThreadPool{4};

		// ----- Latency Metrics -----
	  private:
		size_t m_MaxDurationsToStore = 1000;		  // Maximum number of durations to store for averaging
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

		// ----- Private Structs -----
	  private:
		struct FaceBuildDesc
		{
			Face face;

			const glm::vec3* v[4];
			const uint8_t* o[4];

			bool reverseWinding = false; // vertices should be added in reverse order (for correct backface culling)
		};

		struct PointsAndOcclusion
		{
			glm::vec3 p000, p001, p010, p011, p100, p101, p110, p111;
			uint8_t o000, o001, o010, o011, o100, o101, o110, o111;
		};

		// ----- Private Methods -----
	  private:
		void UpdateChunkMesh(const std::shared_ptr<ChunkMesh> chunkMesh);

		void BuildOcclusionMap(const std::shared_ptr<SubChunkMesh> subMesh,
							   const int subChunkIndex,
							   const std::shared_ptr<Chunk>& chunk,
							   const std::shared_ptr<Chunk>& adjacentPosX,
							   const std::shared_ptr<Chunk>& adjacentNegX,
							   const std::shared_ptr<Chunk>& adjacentPosZ,
							   const std::shared_ptr<Chunk>& adjacentNegZ);

		static void BuildFace(TextureAtlas& textureAtlas,
							  SubChunkMesh& mesh,
							  const BlockState& block,
							  const BlockTextures& blockTextures,
							  const FaceBuildDesc& faceDesc);

		static void AddFace(SubChunkMesh& mesh,
							const FaceBuildDesc& f,
							const BlockState& block,
							const FaceTexture& faceTexture,
							const TextureAtlas::AtlasEntry& uv,
							BlockState::RotationType rotationType);

		PointsAndOcclusion GetPointsAndOcclusion(
			const BlockTextures& blockTextures, SubChunkMesh* mesh, const int lx, const int wy, const int lz);
		PointsAndOcclusion GetPointsAndOcclusionForBlock(SubChunkMesh* mesh, const int lx, const int wy, const int lz);
		PointsAndOcclusion GetPointsAndOcclusionForCross(SubChunkMesh* mesh, const int lx, const int wy, const int lz);

		std::vector<FaceBuildDesc> GetFaceBuildDescs(const BlockTextures& blockTextures, const PointsAndOcclusion& pao);
		std::vector<FaceBuildDesc> GetBlockFaceBuildDescs(const PointsAndOcclusion& pao);
		std::vector<FaceBuildDesc> GetCrossFaceBuildDescs(const PointsAndOcclusion& pao);
	};
} // namespace onion::voxel
