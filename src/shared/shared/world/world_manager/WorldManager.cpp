#include "WorldManager.hpp"

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

	void WorldManager::AddChunk(const std::shared_ptr<void>& chunkData)
	{
		ChunkAdded.Trigger(chunkData);
	}

	void WorldManager::RemoveChunk(const std::shared_ptr<void>& chunkData)
	{
		ChunkRemoved.Trigger(chunkData);
	}
} // namespace onion::voxel
