#include "WorldManager.hpp"

#include "../../utils/Utils.hpp"

namespace onion::voxel
{
	WorldManager::WorldManager() {}

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
} // namespace onion::voxel
