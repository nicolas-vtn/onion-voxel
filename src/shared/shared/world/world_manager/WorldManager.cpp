#include "WorldManager.hpp"

#include <shared/utils/Utils.hpp>

#include <iostream>

namespace onion::voxel
{
	WorldManager::WorldManager()
	{
		// Subscibe to it's own events
		SubscribeToInternalEvents();

		// Start periodic task to request missing chunks (chunks that should be loaded but are not) every second
		m_TimerRequestMissingChunks.setTimeoutFunction([this]() { RequestAllMissingChunks(); });
		m_TimerRequestMissingChunks.setElapsedPeriod(std::chrono::seconds(1));
		m_TimerRequestMissingChunks.Start();
	}

	WorldManager::~WorldManager()
	{
		m_InternalEventHandles.clear();
		m_TimerRequestMissingChunks.Stop();
	}

	void WorldManager::LoadWorld(const std::filesystem::path& worldPath)
	{
		m_CurrentWorldPath = worldPath;
	}

	void WorldManager::UnloadWorld()
	{
		m_CurrentWorldPath.clear();
	}

	std::shared_ptr<Chunk> WorldManager::GetChunk(const glm::ivec2& chunkPosition) const
	{
		std::shared_lock lock(m_MutexChunks);
		auto it = m_Chunks.find(chunkPosition);
		if (it != m_Chunks.end())
		{
			return it->second;
		}
		else
		{
			return nullptr; // Chunk not found
		}
	}

	std::unordered_map<glm::ivec2, std::shared_ptr<Chunk>, IVec2Hash> WorldManager::GetAllChunks() const
	{
		std::shared_lock lock(m_MutexChunks);
		return m_Chunks; // Return a copy of the chunks map
	}

	void WorldManager::AddChunk(const std::shared_ptr<Chunk> chunk)
	{
		std::unique_lock lock(m_MutexChunks);
		m_Chunks[chunk->GetPosition()] = chunk;

		// Unlock before triggering event
		lock.unlock();

		// Trigger event
		ChunkAdded.Trigger(chunk);
	}

	void WorldManager::AddChunk(const std::shared_ptr<Chunk> chunk, const std::vector<Block>& outOfBoundsBlocks)
	{
		// Converts world position to chunk position + local position.
		std::unordered_map<glm::ivec2, std::vector<Block>, IVec2Hash> outOfBoundsBlocksByChunk;
		for (const auto& block : outOfBoundsBlocks)
		{
			glm::ivec2 chunkPos = Utils::WorldToChunkPosition(block.Position);
			outOfBoundsBlocksByChunk[chunkPos].push_back(block);
		}

		// Add out-of-bounds blocks to the map so they can be placed in the chunk when it is added to the world
		{
			std::unique_lock lock(m_MutexOutOfBoundsBlocks);
			for (const auto& [chunkPos, blocks] : outOfBoundsBlocksByChunk)
			{
				m_OutOfBoundsBlocks[chunkPos].insert(m_OutOfBoundsBlocks[chunkPos].end(), blocks.begin(), blocks.end());
			}
		}

		// Info : AddChunk triggers the ChunkAdded event, which will call PlaceOutOfBoundsBlocks to place the out-of-bounds blocks in the chunk when it is added to the world
		AddChunk(chunk);
	}

	void WorldManager::RemoveChunk(const glm::ivec2& chunkPosition)
	{
		std::unique_lock lock(m_MutexChunks);

		auto it = m_Chunks.find(chunkPosition);
		if (it != m_Chunks.end())
		{
			// Copy chunk
			std::shared_ptr<Chunk> chunk = it->second;

			// Remove chunk from map
			m_Chunks.erase(it);

			// Unlock before triggering event
			lock.unlock();

			// Trigger event
			ChunkRemoved.Trigger(chunk);
		}
	}

	void WorldManager::RemoveAllChunks()
	{
		std::vector<std::shared_ptr<Chunk>> chunksToRemove;

		{
			std::unique_lock lock(m_MutexChunks);
			chunksToRemove.reserve(m_Chunks.size());
			for (const auto& [pos, chunk] : m_Chunks)
			{
				chunksToRemove.push_back(chunk);
			}
			m_Chunks.clear();
		}

		for (const auto& chunk : chunksToRemove)
		{
			ChunkRemoved.Trigger(chunk);
		}
	}

	void WorldManager::ClearWorld()
	{
		RemoveAllChunks();

		{
			std::unique_lock lock(m_MutexPlayersPosition);
			m_PlayersPosition.clear();
		}

		{
			std::unique_lock lock(m_MutexOutOfBoundsBlocks);
			m_OutOfBoundsBlocks.clear();
		}
	}

	bool WorldManager::IsChunkLoaded(const glm::ivec2& chunkPosition) const
	{
		std::shared_lock lock(m_MutexChunks);
		return m_Chunks.find(chunkPosition) != m_Chunks.end();
	}

	std::filesystem::path WorldManager::GetCurrentWorldPath() const
	{
		return m_CurrentWorldPath;
	}

	uint32_t WorldManager::GetSeed() const
	{
		return m_Seed;
	}

	void WorldManager::SetSeed(uint32_t seed)
	{
		m_Seed = seed;
		SeedChanged.Trigger(seed);
	}

	std::unordered_map<uint32_t, glm::vec3> WorldManager::GetPlayersPosition() const
	{
		std::shared_lock lock(m_MutexPlayersPosition);
		return m_PlayersPosition; // Return a copy of the players position map
	}

	void WorldManager::SetPlayerPosition(uint32_t ClientHandle, const glm::vec3& position)
	{
		glm::vec3 previousPosition;

		{
			std::lock_guard lock(m_MutexPlayersPosition);

			previousPosition = m_PlayersPosition[ClientHandle];

			if (position == previousPosition)
			{
				return; // Player hasn't moved.
			}

			m_PlayersPosition[ClientHandle] = position;
		}

		glm::ivec2 previousChunkPosition = Utils::WorldToChunkPosition(previousPosition);
		glm::ivec2 newChunkPosition = Utils::WorldToChunkPosition(position);

		if (previousChunkPosition != newChunkPosition)
		{
			PlayerChangedChunk.Trigger({ClientHandle, previousChunkPosition, newChunkPosition});
		}
	}

	uint8_t WorldManager::GetChunkPersistanceDistance() const
	{
		return m_ChunkPersistanceDistance;
	}

	void WorldManager::SetChunkPersistanceDistance(uint8_t distance)
	{
		m_ChunkPersistanceDistance = distance;
		RemoveDistantChunks();
	}

	void WorldManager::SetServerSimulationDistance(uint8_t distance)
	{
		m_ServerSimulationDistance = distance;
	}

	bool WorldManager::IsTriggeringEventMissingChunks() const
	{
		return m_TimerRequestMissingChunks.isRunning();
	}

	void WorldManager::SetTriggeringEventMissingChunks(bool trigger)
	{
		if (trigger)
		{
			m_TimerRequestMissingChunks.Start();
		}
		else
		{
			m_TimerRequestMissingChunks.Stop();
		}
	}

	void WorldManager::SubscribeToInternalEvents()
	{
		m_InternalEventHandles.push_back(PlayerChangedChunk.Subscribe([this](const PlayerChangedChunkEventArgs& args)
																	  { Handle_PlayerChangedChunk(args); }));

		m_InternalEventHandles.push_back(
			ChunkAdded.Subscribe([this](const std::shared_ptr<Chunk>& chunk) { Handle_ChunkAdded(chunk); }));
	}

	void WorldManager::Handle_PlayerChangedChunk(const PlayerChangedChunkEventArgs& args)
	{
		std::cout << "Player " << args.ClientHandle << " changed chunk from " << args.OldChunkPosition.x << ", "
				  << args.OldChunkPosition.y << " to " << args.NewChunkPosition.x << ", " << args.NewChunkPosition.y
				  << std::endl;

		RemoveDistantChunks();
		RequestMissingChunksAround(args.NewChunkPosition);
	}

	BlockState WorldManager::GetBlock(const glm::ivec3& worldPosition) const
	{
		// Calculate chunk position and local position within chunk
		glm::ivec2 chunkPosition = Utils::WorldToChunkPosition(worldPosition);
		glm::ivec3 localPosition = Utils::WorldToLocalPosition(worldPosition);

		std::shared_ptr<Chunk> chunk = GetChunk(chunkPosition);
		if (chunk)
		{
			return chunk->GetBlock(localPosition);
		}
		else
		{
			return BlockState(); // Air block if chunk not found
		}
	}

	bool WorldManager::SetBlock(const Block& block, BlocksChangedEventArgs::eOrigin origin, bool notify)
	{
		int blockPlaced = SetBlocks({block}, origin, notify);
		return blockPlaced > 0;
	}

	size_t
	WorldManager::SetBlocks(const std::vector<Block>& blocks, BlocksChangedEventArgs::eOrigin origin, bool notify)
	{
		std::unordered_map<glm::ivec2, std::vector<std::pair<glm::ivec3, Block>>, IVec2Hash> blocksByChunk;
		std::vector<Block> blocksToNotify;

		// Group blocks by chunk
		for (const auto& block : blocks)
		{
			glm::ivec2 chunkPos = Utils::WorldToChunkPosition(block.Position);
			glm::ivec3 localPos = Utils::WorldToLocalPosition(block.Position);
			blocksByChunk[chunkPos].emplace_back(localPos, block);
		}

		// Set blocks for each chunk
		for (const auto& [chunkPos, blocksInChunk] : blocksByChunk)
		{
			std::shared_ptr<Chunk> chunk = GetChunk(chunkPos);
			if (chunk)
			{
				for (const auto& [localPos, block] : blocksInChunk)
				{
					BlockState currentBlock = chunk->GetBlock(localPos);
					if (currentBlock != block.State)
					{
						chunk->SetBlock(localPos, block.State);

						const glm::ivec3 worldPos = Utils::LocalToWorldPosition(localPos, chunkPos);
						blocksToNotify.push_back(block);
					}
				}
			}
			else
			{
				// If chunk is not loaded, add blocks to out-of-bounds map so they can be placed when the chunk is loaded
				std::unique_lock lock(m_MutexOutOfBoundsBlocks);

				for (const auto& [localPos, block] : blocksInChunk)
				{
					m_OutOfBoundsBlocks[chunkPos].push_back(block);
				}
			}
		}

		int numBlocksSet = blocksToNotify.size();
		if (notify && numBlocksSet > 0)
		{
			BlocksChangedEventArgs args;
			args.ChangedBlocks = std::move(blocksToNotify);
			args.Origin = origin;
			BlocksChanged.Trigger(args);
		}

		return numBlocksSet;
	}

	void WorldManager::PlaceOutOfBoundsBlocks()
	{
		std::vector<std::pair<std::shared_ptr<Chunk>, std::vector<Block>>> work;
		std::vector<Block> blocksPlaced;

		{
			std::unique_lock lock(m_MutexOutOfBoundsBlocks);

			for (auto it = m_OutOfBoundsBlocks.begin(); it != m_OutOfBoundsBlocks.end();)
			{
				auto chunk = GetChunk(it->first);

				if (chunk)
				{
					work.emplace_back(chunk, std::move(it->second));
					it = m_OutOfBoundsBlocks.erase(it);
				}
				else
				{
					++it;
				}
			}
		}

		// ---- No locks held here ----

		for (auto& [chunk, blocks] : work)
		{
			glm::ivec2 chunkPos = chunk->GetPosition();

			for (auto& block : blocks)
			{
				glm::ivec3 localPos = Utils::WorldToLocalPosition(block.Position);
				chunk->SetBlock(localPos, block.State);

				const glm::ivec3 worldPos = Utils::LocalToWorldPosition(localPos, chunkPos);
				blocksPlaced.push_back(block);
			}
		}

		if (!blocksPlaced.empty())
		{
			BlocksChangedEventArgs args;
			args.ChangedBlocks = std::move(blocksPlaced);
			args.Origin = BlocksChangedEventArgs::eOrigin::OutOfBoundsPlaced;
			BlocksChanged.Trigger(args);
		}
	}

	void WorldManager::Handle_ChunkAdded(const std::shared_ptr<Chunk>& chunk)
	{
		PlaceOutOfBoundsBlocks();
	}

	void WorldManager::RequestAllMissingChunks() const
	{
		// Request missing chunks around each player
		std::vector<glm::ivec2> missingChunks;
		{
			std::shared_lock lock(m_MutexPlayersPosition);
			for (const auto& [clientHandle, position] : m_PlayersPosition)
			{
				missingChunks.emplace_back(Utils::WorldToChunkPosition(position));
			}
		}

		for (const auto& chunkPos : missingChunks)
		{
			RequestMissingChunksAround(chunkPos);
		}
	}

	void WorldManager::RequestMissingChunksAround(const glm::ivec2& chunkPosition) const
	{
		std::vector<glm::ivec2> missingChunks;
		auto playersPosition = GetPlayersPosition();
		std::unordered_map<glm::ivec2, std::shared_ptr<Chunk>, IVec2Hash> chunks = GetAllChunks();

		// For each player, check the chunks within the persistance distance and mark them as missing if they are not loaded
		int persistanceDistance = std::min(GetChunkPersistanceDistance(), m_ServerSimulationDistance.load());

		for (const auto& [clientHandle, playerPos] : playersPosition)
		{
			glm::ivec2 playerChunkPos = Utils::WorldToChunkPosition(playerPos);

			for (int x = -persistanceDistance; x <= persistanceDistance; x++)
			{
				for (int y = -persistanceDistance; y <= persistanceDistance; y++)
				{
					glm::ivec2 chunkPos = playerChunkPos + glm::ivec2(x, y);
					if (chunks.find(chunkPos) == chunks.end())
					{
						missingChunks.push_back(chunkPos); // Mark chunk as missing if it is not loaded
					}
				}
			}
		}

		// Sorts missing chunks from closest to farthest from the given chunk position
		std::sort(missingChunks.begin(),
				  missingChunks.end(),
				  [&chunkPosition](const glm::ivec2& a, const glm::ivec2& b)
				  {
					  return glm::distance(glm::vec2(a), glm::vec2(chunkPosition)) <
						  glm::distance(glm::vec2(b), glm::vec2(chunkPosition));
				  });

		// Trigger event to request missing chunks to be loaded
		if (!missingChunks.empty())
		{
			MissingChunksRequested.Trigger(missingChunks);
		}
	}

	void WorldManager::RemoveDistantChunks()
	{
		auto playersPosition = GetPlayersPosition();

		// Initialize all chunks as not to be kept
		std::unordered_map<glm::ivec2, bool, IVec2Hash> chunksToKeep;
		{
			std::shared_lock lock(m_MutexChunks);
			for (const auto& [chunkPos, chunk] : m_Chunks)
			{
				chunksToKeep[chunkPos] = false;
			}
		}

		// Mark chunks that are within the persistance distance of any player to be kept
		int persistanceDistance = GetChunkPersistanceDistance();

		for (const auto& [clientHandle, playerPos] : playersPosition)
		{
			glm::ivec2 playerChunkPos = Utils::WorldToChunkPosition(playerPos);

			for (int x = -persistanceDistance; x <= persistanceDistance; ++x)
			{
				for (int y = -persistanceDistance; y <= persistanceDistance; ++y)
				{
					glm::ivec2 chunkPos = playerChunkPos + glm::ivec2(x, y);
					chunksToKeep[chunkPos] = true; // Mark chunk as to be kept
				}
			}
		}

		// Collect chunks that are not to be kept for removal
		std::vector<glm::ivec2> chunksToRemove;
		for (const auto& [chunkPos, keep] : chunksToKeep)
		{
			if (!keep)
			{
				chunksToRemove.push_back(chunkPos); // Mark chunk for removal if it is not to be kept
			}
		}

		// Remove chunks that are not to be kept
		for (const auto& chunkPos : chunksToRemove)
		{
			RemoveChunk(chunkPos);
		}
	}
} // namespace onion::voxel
