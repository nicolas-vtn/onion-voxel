#pragma once

#include <glm/glm.hpp>

#include <filesystem>
#include <memory>
#include <shared_mutex>
#include <unordered_map>

#include <onion/Event.hpp>

#include "../chunk/Chunk.hpp"

namespace onion::voxel
{
	struct IVec2Hash
	{
		size_t operator()(const glm::ivec2& v) const noexcept
		{
			return (static_cast<uint64_t>(v.x) << 32) ^ static_cast<uint32_t>(v.y);
		}
	};

	class WorldManager
	{
		// ----- Constructor / Destructor -----
	  public:
		WorldManager();
		~WorldManager();

		// ----- Public API -----
	  public:
		void LoadWorld(const std::filesystem::path& worldPath);
		void UnloadWorld();

		void AddChunk(const std::shared_ptr<Chunk> chunk);
		void RemoveChunk(const glm::ivec2& chunkPosition);

		void RemoveAllChunks();

		Block GetBlock(const glm::ivec3& worldPosition) const;

		bool IsChunkLoaded(const glm::ivec2& chunkPosition) const;
		std::shared_ptr<Chunk> GetChunk(const glm::ivec2& chunkPosition) const;
		std::unordered_map<glm::ivec2, std::shared_ptr<Chunk>, IVec2Hash> GetAllChunks() const;

		// ----- Events -----
	  public:
		Event<std::shared_ptr<Chunk>> ChunkAdded;
		Event<std::shared_ptr<Chunk>> ChunkRemoved;

		// ----- Members -----
	  private:
		std::filesystem::path m_CurrentWorldPath;

		mutable std::shared_mutex m_MutexChunks;
		std::unordered_map<glm::ivec2, std::shared_ptr<Chunk>, IVec2Hash> m_Chunks;
	};
} // namespace onion::voxel
