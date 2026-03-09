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
		void AddChunk(const std::shared_ptr<Chunk> chunk,
					  const std::unordered_map<glm::ivec3, Block>& outOfBoundsBlocks);
		void RemoveChunk(const glm::ivec2& chunkPosition);

		void RemoveAllChunks();

		Block GetBlock(const glm::ivec3& worldPosition) const;
		bool SetBlock(const glm::ivec3& worldPosition, const Block& block, bool notify = true);
		size_t SetBlocks(const std::vector<std::pair<glm::ivec3, Block>>& blocks, bool notify = true);

		bool IsChunkLoaded(const glm::ivec2& chunkPosition) const;
		std::shared_ptr<Chunk> GetChunk(const glm::ivec2& chunkPosition) const;
		std::unordered_map<glm::ivec2, std::shared_ptr<Chunk>, IVec2Hash> GetAllChunks() const;

		// ----- Getters / Setters -----
	  public:
		std::filesystem::path GetCurrentWorldPath() const;

		uint32_t GetSeed() const;
		void SetSeed(uint32_t seed);

		// ----- Events -----
	  public:
		Event<const uint32_t&> SeedChanged;
		Event<std::shared_ptr<Chunk>> ChunkAdded;
		Event<std::shared_ptr<Chunk>> ChunkRemoved;
		Event<const std::vector<std::pair<glm::ivec3, Block>>&> BlocksChanged;

		// ----- Members -----
	  private:
		std::filesystem::path m_CurrentWorldPath;
		std::atomic_uint32_t m_Seed{1};

		mutable std::shared_mutex m_MutexOutOfBoundsBlocks;
		// Map of chunk position to list of out-of-bounds blocks that should be added to the chunk when it is added to the world
		std::unordered_map<glm::ivec2, std::vector<std::pair<glm::ivec3, Block>>, IVec2Hash> m_OutOfBoundsBlocks;
		void PlaceOutOfBoundsBlocks();
		EventHandle m_ChunkAddedEventHandle;
		void Handle_ChunkAdded(const std::shared_ptr<Chunk>& chunk);

		mutable std::shared_mutex m_MutexChunks;
		std::unordered_map<glm::ivec2, std::shared_ptr<Chunk>, IVec2Hash> m_Chunks;
	};
} // namespace onion::voxel
