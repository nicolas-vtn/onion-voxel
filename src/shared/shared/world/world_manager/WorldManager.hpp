#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <filesystem>
#include <memory>
#include <shared_mutex>
#include <unordered_map>

#include <onion/Event.hpp>
#include <onion/Timer.hpp>

#include <shared/entities/entity_manager/EntityManager.hpp>
#include <shared/world/chunk/Chunk.hpp>
#include <shared/world/world_generator/WorldGenerator.hpp>
#include <shared/world/world_save/WorldSave.hpp>

namespace onion::voxel
{
	class WorldManager
	{
		// ----- Structs -----
	  public:
		struct PlayerChangedChunkEventArgs
		{
			std::string PlayerUUID;
			glm::ivec2 OldChunkPosition{};
			glm::ivec2 NewChunkPosition{};
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
		WorldManager(const std::filesystem::path& worldDirectory, bool readonly);
		~WorldManager();

		// ----- Public API -----
	  public:
		void AddChunk(const std::shared_ptr<Chunk> chunk);
		void AddChunk(const std::shared_ptr<Chunk> chunk, const std::vector<Block>& outOfBoundsBlocks);
		void RemoveChunk(const glm::ivec2& chunkPosition);

		void RemoveDistantChunks();
		void RemoveAllChunks();

		void ClearWorld();

		BlockState GetBlock(const glm::ivec3& worldPosition) const;
		bool SetBlock(const Block& block, BlocksChangedEventArgs::eOrigin origin, bool notify = true);
		size_t SetBlocks(const std::vector<Block>& blocks, BlocksChangedEventArgs::eOrigin origin, bool notify = true);

		bool IsChunkLoaded(const glm::ivec2& chunkPosition) const;
		std::shared_ptr<Chunk> GetChunk(const glm::ivec2& chunkPosition) const;
		std::unordered_map<glm::ivec2, std::shared_ptr<Chunk>> GetAllChunks() const;

		void AddPlayer(const std::shared_ptr<Player>& player);
		std::shared_ptr<Player> LoadPlayer(const std::string& playerUUID);
		void RemovePlayer(const std::string& playerUUID);

		void RequestAllMissingChunks();

		// ----- Getters / Setters -----
	  public:
		uint32_t GetSeed() const;
		void SetSeed(uint32_t seed);

		std::unordered_map<std::string, glm::vec3> GetPlayersPosition() const;

		std::shared_ptr<Player> GetPlayer(const std::string& uuid) const;

		uint8_t GetChunkPersistanceDistance() const;
		void SetChunkPersistanceDistance(uint8_t distance);

		void SetChunkLoadingDistance(uint8_t distance);

		bool IsTriggeringEventMissingChunks() const;
		void SetTriggeringEventMissingChunks(bool trigger);

		void SetPlayerPosition(const std::string& playerUUID, const glm::vec3& position);
		void UpdatePlayer(const std::shared_ptr<Player>& player);
		void UpdateEntities(const std::vector<std::shared_ptr<Entity>>& entities);

		std::vector<std::shared_ptr<Entity>> GetAllEntities() const;
		std::unordered_map<std::string, std::shared_ptr<Player>> GetAllPlayers() const;

		void SetSingleplayerPlayerUUID(const std::string& playerUUID);
		std::string GetSingleplayerPlayerUUID() const;

		// ----- Events -----
	  public:
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
		void Handle_ChunkGenerated(const WorldGenerator::GenChunk& genChunk);
		void Handle_ChunkRemoved(const std::shared_ptr<Chunk>& chunk);

		void Handle_PlayerAdded(const std::shared_ptr<Player>& player);
		void Handle_PlayerRemoved(const std::shared_ptr<Player>& player);

		// ----- Members -----
	  private:
		std::atomic_uint8_t m_ChunkPersistanceDistance{5};
		std::atomic_uint8_t m_ChunkLoadingDistance{5};

		std::unique_ptr<WorldGenerator> m_WorldGenerator;
		std::unique_ptr<WorldSave> m_WorldSave;

		std::shared_ptr<EntityManager> m_EntityManager = std::make_shared<EntityManager>();

		mutable std::shared_mutex m_MutexOutOfBoundsBlocks;
		// Map of chunk position to list of out-of-bounds blocks that should be added to the chunk when it is added to the world
		std::unordered_map<glm::ivec2, std::vector<Block>> m_OutOfBoundsBlocks;

		mutable std::shared_mutex m_MutexChunks;
		std::unordered_map<glm::ivec2, std::shared_ptr<Chunk>> m_Chunks;

		mutable std::shared_mutex m_MutexSingleplayer;
		std::string m_SingleplayerPlayerUUID;

		// ----- Private Methods -----
	  private:
		void RequestMissingChunksAsync(const std::vector<glm::ivec2>& chunkPositions);

		// ----- Periodic Tasks -----
	  private:
		void PlaceOutOfBoundsBlocks();

		Timer m_TimerRequestMissingChunks;
	};
} // namespace onion::voxel
