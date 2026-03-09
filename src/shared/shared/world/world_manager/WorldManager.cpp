#include "WorldManager.hpp"

#include "../../utils/Utils.hpp"

namespace onion::voxel
{
	WorldManager::WorldManager()
	{
		// Subscibe to it's own event to handle out-of-bounds blocks when a chunk is added
		m_ChunkAddedEventHandle =
			ChunkAdded.Subscribe([this](const std::shared_ptr<Chunk>& chunk) { Handle_ChunkAdded(chunk); });
	}

	WorldManager::~WorldManager() {}

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

	void WorldManager::AddChunk(const std::shared_ptr<Chunk> chunk,
								const std::unordered_map<glm::ivec3, Block>& outOfBoundsBlocks)
	{
		// Converts world position to chunk position + local position.
		std::unordered_map<glm::ivec2, std::vector<std::pair<glm::ivec3, Block>>, IVec2Hash> outOfBoundsBlocksByChunk;
		for (const auto& [worldPos, block] : outOfBoundsBlocks)
		{
			glm::ivec2 chunkPos = Utils::WorldToChunkPosition(worldPos);
			glm::ivec3 localPos = Utils::WorldToLocalPosition(worldPos);
			outOfBoundsBlocksByChunk[chunkPos].emplace_back(localPos, block);
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

	Block WorldManager::GetBlock(const glm::ivec3& worldPosition) const
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
			return Block(); // Air block if chunk not found
		}
	}

	bool WorldManager::SetBlock(const glm::ivec3& worldPosition, const Block& block, bool notify)
	{
		// Calculate chunk position and local position within chunk
		glm::ivec2 chunkPosition = Utils::WorldToChunkPosition(worldPosition);

		std::shared_ptr<Chunk> chunk = GetChunk(chunkPosition);

		glm::ivec3 localPosition = Utils::WorldToLocalPosition(worldPosition);
		if (chunk)
		{
			if (chunk->GetBlock(localPosition) == block)
			{
				return false; // Block is already the same, no need to set or trigger event
			}

			chunk->SetBlock(localPosition, block);

			if (notify)
			{
				BlocksChanged.Trigger(std::vector<std::pair<glm::ivec3, Block>>{{worldPosition, block}});
			}

			return true;
		}
		else
		{
			return false; // Chunk not found
		}
	}

	size_t WorldManager::SetBlocks(const std::vector<std::pair<glm::ivec3, Block>>& blocks, bool notify)
	{
		std::unordered_map<glm::ivec2, std::vector<std::pair<glm::ivec3, Block>>, IVec2Hash> blocksByChunk;
		std::vector<std::pair<glm::ivec3, Block>> blocksToNotify;

		// Group blocks by chunk
		for (const auto& [worldPos, block] : blocks)
		{
			glm::ivec2 chunkPos = Utils::WorldToChunkPosition(worldPos);
			glm::ivec3 localPos = Utils::WorldToLocalPosition(worldPos);
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
					Block currentBlock = chunk->GetBlock(localPos);
					if (currentBlock != block)
					{
						chunk->SetBlock(localPos, block);

						const glm::ivec3 worldPos = Utils::LocalToWorldPosition(localPos, chunkPos);
						blocksToNotify.emplace_back(worldPos, block);
					}
				}
			}
		}

		if (notify && blocksToNotify.size() > 0)
		{
			BlocksChanged.Trigger(blocksToNotify);
		}

		return blocksToNotify.size();
	}

	void WorldManager::PlaceOutOfBoundsBlocks()
	{
		std::vector<std::pair<glm::ivec3, Block>> blocksPlaced;
		{
			std::unique_lock lock(m_MutexOutOfBoundsBlocks);
			for (auto it = m_OutOfBoundsBlocks.begin(); it != m_OutOfBoundsBlocks.end();)
			{
				const glm::ivec2& chunkPos = it->first;
				std::shared_ptr<Chunk> chunk = GetChunk(chunkPos);
				if (chunk)
				{
					// Place out-of-bounds blocks in the chunk
					for (const auto& [localPos, block] : it->second)
					{
						chunk->SetBlock(localPos, block);
						const glm::ivec3 worldPos = Utils::LocalToWorldPosition(localPos, chunkPos);
						blocksPlaced.emplace_back(worldPos, block);
					}
					// Remove the entry from the map after placing the blocks
					it = m_OutOfBoundsBlocks.erase(it);
				}
				else
				{
					++it; // Move to the next entry if the chunk is not loaded yet
				}
			}
		}

		if (!blocksPlaced.empty())
		{
			BlocksChanged.Trigger(blocksPlaced);
		}
	}

	void WorldManager::Handle_ChunkAdded(const std::shared_ptr<Chunk>& chunk)
	{
		PlaceOutOfBoundsBlocks();
	}
} // namespace onion::voxel
