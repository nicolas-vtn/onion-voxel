#pragma once

#include <glm/glm.hpp>

#include <filesystem>
#include <memory>
#include <shared_mutex>
#include <unordered_map>

#include <onion/Event.hpp>
#include <onion/Timer.hpp>

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
		// ----- Structs -----
	  public:
		struct PlayerChangedChunkEventArgs
		{
			uint32_t ClientHandle{0};
			glm::ivec2 OldChunkPosition;
			glm::ivec2 NewChunkPosition;
		};

		struct BlocksChangedEventArgs
		{
			enum class eOrigin
			{
				Unknown,
				PlayerAction,
				ServerRequest,
				ClientRequest,
				OutOfBoundsPlaced,
			};

			std::vector<Block> ChangedBlocks;
			eOrigin Origin{eOrigin::Unknown};
		};

		// ----- Constructor / Destructor -----
	  public:
		WorldManager();
		~WorldManager();

		// ----- Public API -----
	  public:
		void LoadWorld(const std::filesystem::path& worldPath);
		void UnloadWorld();

		void AddChunk(const std::shared_ptr<Chunk> chunk);
		void AddChunk(const std::shared_ptr<Chunk> chunk, const std::vector<Block>& outOfBoundsBlocks);
		void RemoveChunk(const glm::ivec2& chunkPosition);

		void RemoveAllChunks();

		void ClearWorld();

		BlockState GetBlock(const glm::ivec3& worldPosition) const;
		bool SetBlock(const Block& block, BlocksChangedEventArgs::eOrigin origin, bool notify = true);
		size_t SetBlocks(const std::vector<Block>& blocks, BlocksChangedEventArgs::eOrigin origin, bool notify = true);

		bool IsChunkLoaded(const glm::ivec2& chunkPosition) const;
		std::shared_ptr<Chunk> GetChunk(const glm::ivec2& chunkPosition) const;
		std::unordered_map<glm::ivec2, std::shared_ptr<Chunk>, IVec2Hash> GetAllChunks() const;

		// ----- Getters / Setters -----
	  public:
		std::filesystem::path GetCurrentWorldPath() const;

		uint32_t GetSeed() const;
		void SetSeed(uint32_t seed);

		std::unordered_map<uint32_t, glm::vec3> GetPlayersPosition() const;
		void SetPlayerPosition(uint32_t ClientHandle, const glm::vec3& position);

		uint8_t GetChunkPersistanceDistance() const;
		void SetChunkPersistanceDistance(uint8_t distance);

		void SetServerSimulationDistance(uint8_t distance);

		bool IsTriggeringEventMissingChunks() const;
		void SetTriggeringEventMissingChunks(bool trigger);

		// ----- Events -----
	  public:
		Event<const uint32_t&> SeedChanged;
		Event<std::shared_ptr<Chunk>> ChunkAdded;
		Event<std::shared_ptr<Chunk>> ChunkRemoved;
		Event<const BlocksChangedEventArgs&> BlocksChanged;
		Event<const std::vector<glm::ivec2>&> MissingChunksRequested;

	  private:
		std::vector<EventHandle> m_InternalEventHandles;
		void SubscribeToInternalEvents();

		Event<const PlayerChangedChunkEventArgs&> PlayerChangedChunk;

		void Handle_PlayerChangedChunk(const PlayerChangedChunkEventArgs& args);
		void Handle_ChunkAdded(const std::shared_ptr<Chunk>& chunk);

		// ----- Members -----
	  private:
		std::filesystem::path m_CurrentWorldPath;
		std::atomic_uint32_t m_Seed{1};
		std::atomic_uint8_t m_ChunkPersistanceDistance{5};
		std::atomic_uint8_t m_ServerSimulationDistance{5};

		mutable std::shared_mutex m_MutexPlayersPosition;
		std::unordered_map<uint32_t, glm::vec3> m_PlayersPosition;

		mutable std::shared_mutex m_MutexOutOfBoundsBlocks;
		// Map of chunk position to list of out-of-bounds blocks that should be added to the chunk when it is added to the world
		std::unordered_map<glm::ivec2, std::vector<Block>, IVec2Hash> m_OutOfBoundsBlocks;

		mutable std::shared_mutex m_MutexChunks;
		std::unordered_map<glm::ivec2, std::shared_ptr<Chunk>, IVec2Hash> m_Chunks;

		// ----- Periodic Tasks -----
	  private:
		void PlaceOutOfBoundsBlocks();

		Timer m_TimerRequestMissingChunks;
		void RequestAllMissingChunks() const;
		void RequestMissingChunksAround(const glm::ivec2& chunkPosition) const;

		void RemoveDistantChunks();
	};
} // namespace onion::voxel
