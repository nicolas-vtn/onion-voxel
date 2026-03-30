#pragma once

#include <filesystem>
#include <mutex>
#include <unordered_map>

#include <onion/Timer.hpp>

#include <shared/world/chunk/Chunk.hpp>

#include "WorldInfos.hpp"

namespace onion::voxel
{
	class WorldSave
	{
		// ----- Constructor / Destructor -----
	  public:
		WorldSave(const std::filesystem::path& saveDirectory);
		~WorldSave();

		// ----- Public API -----
	  public:
		uint32_t GetSeed() const;
		WorldGenerator::eWorldGenerationType GetWorldGenerationType() const;

		static void CreateWorld(const std::filesystem::path& saveDirectory, const WorldInfos& infos);
		static bool GetWorldInfos(const std::filesystem::path& saveDirectory, WorldInfos& outInfos);

		void SaveChunkAsync(const std::shared_ptr<Chunk>& chunk);
		std::shared_ptr<Chunk> LoadChunk(const glm::ivec2& chunkPosition);

		// ----- Private Members -----
	  private:
		const std::filesystem::path m_SaveDirectory;

		static inline const std::string s_InfosFileName = "infos.json";
		static inline const std::string s_ChunksDirectoryName = "chunks";
		static inline const std::string s_RegionDirectoryNamePrefix = "region_";
		static inline const uint8_t s_RegionSizeInChunks = 32;
		WorldInfos m_Infos;

		mutable std::mutex m_MutexDiskAccess;

		mutable std::mutex m_MutexChunksToSave;
		std::unordered_map<glm::ivec2, std::shared_ptr<Chunk>> m_ChunksToSave;

		Timer m_TimerSaveChunks;
		int m_SaveChunksPeriodSeconds = 5;
		void SaveChunksPeriodically();

		// ----- Private Methods -----
	  private:
		static void SaveInfos(const std::filesystem::path& saveDirectory, const WorldInfos& infos);
		static WorldInfos LoadInfos(const std::filesystem::path& saveDirectory);
		static std::string GetChunkFileName(const glm::ivec2& chunkPosition);
		static std::string GetRegionDirectoryName(const glm::ivec2& chunkPosition);
		static std::filesystem::path GetChunkFilePath(const std::filesystem::path& saveDirectory,
													  const glm::ivec2& chunkPosition);
	};
} // namespace onion::voxel
