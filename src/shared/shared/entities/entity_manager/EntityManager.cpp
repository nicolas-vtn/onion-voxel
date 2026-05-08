#include "EntityManager.hpp"

#include <stdexcept>

#include <shared/utils/Utils.hpp>

namespace onion::voxel
{
	EntityManager::EntityManager() = default;
	EntityManager::~EntityManager() = default;

	std::shared_ptr<Player> EntityManager::GetPlayer(const std::string& uuid) const
	{
		std::shared_lock lock(m_MutexPlayers);
		auto it = m_Players.find(uuid);
		if (it != m_Players.end())
		{
			return it->second;
		}
		else
		{
			return nullptr;
		}
	}

	void EntityManager::AddPlayer(const std::shared_ptr<Player>& player)
	{
		{
			std::unique_lock lock(m_MutexPlayers);
			// Throw an exception if a player with the same UUID already exists
			if (m_Players.find(player->UUID) != m_Players.end())
			{
				throw std::runtime_error("Player with UUID " + player->UUID + " already exists.");
			}

			m_Players[player->UUID] = player;
		}

		EvtPlayerAdded.Trigger(player);
	}

	bool EntityManager::RemovePlayer(const std::string& uuid)
	{
		std::unique_lock lock(m_MutexPlayers);
		auto it = m_Players.find(uuid);
		if (it != m_Players.end())
		{
			auto player = it->second; // Store player before erasing to trigger event after unlocking
			m_Players.erase(it);
			lock.unlock(); // Unlock before triggering event to avoid potential deadlocks
			EvtPlayerRemoved.Trigger(player);
			return true;
		}
		return false;
	}

	glm::ivec3 EntityManager::GetPlayerPosition(const std::string& uuid) const
	{
		std::shared_lock lock(m_MutexPlayers);
		auto it = m_Players.find(uuid);
		if (it != m_Players.end())
		{
			return it->second->GetPosition();
		}
		else
		{
			throw std::runtime_error("Player with UUID " + uuid + " not found.");
		}
	}

	std::unordered_map<std::string, glm::vec3> EntityManager::GetAllPlayersPosition() const
	{
		std::shared_lock lock(m_MutexPlayers);
		std::unordered_map<std::string, glm::vec3> positions;
		for (const auto& [uuid, player] : m_Players)
		{
			positions[uuid] = player->GetPosition();
		}
		return positions;
	}

	void EntityManager::SetPlayerPosition(const std::string& uuid, const glm::vec3& newPosition)
	{
		std::unique_lock lock(m_MutexPlayers);
		auto it = m_Players.find(uuid);
		if (it != m_Players.end())
		{
			it->second->SetPosition(newPosition);
		}
		else
		{
			throw std::runtime_error("Player with UUID " + uuid + " not found.");
		}
	}

	void EntityManager::AddEntity(const std::shared_ptr<Entity>& entity)
	{
		std::unique_lock lock(m_MutexEntities);
		auto it = m_Entities.find(entity->UUID);
		if (it != m_Entities.end())
		{
			throw std::runtime_error("Entity with UUID " + entity->UUID + " already exists.");
		}
		m_Entities[entity->UUID] = entity;
	}

	void EntityManager::AddOrUpdateEntity(const std::shared_ptr<Entity>& entity)
	{
		std::unique_lock lock(m_MutexEntities);
		m_Entities[entity->UUID] = entity;
	}

	std::shared_ptr<Entity> EntityManager::GetEntity(const std::string& uuid) const
	{
		std::shared_lock lock(m_MutexEntities);
		auto it = m_Entities.find(uuid);
		if (it != m_Entities.end())
		{
			return it->second;
		}

		return nullptr;
	}

	bool EntityManager::RemoveEntity(const std::string& uuid)
	{
		std::unique_lock lock(m_MutexEntities);
		auto it = m_Entities.find(uuid);
		if (it != m_Entities.end())
		{
			m_Entities.erase(it);
			return true;
		}
		return false;
	}

	void EntityManager::UpdateEntities(const std::vector<std::shared_ptr<Entity>>& entities)
	{
		std::unique_lock lock(m_MutexEntities);
		std::unique_lock lockPlayers(m_MutexPlayers);

		for (const auto& entity : entities)
		{
			if (entity->Type == EntityType::Player)
			{
				// Update player information
				auto player = std::dynamic_pointer_cast<Player>(entity);
				if (player)
				{
					m_Players[player->UUID] = player;
				}
			}
			else
			{
				// Update or add non-player entity
				m_Entities[entity->UUID] = entity;
			}
		}
	}

	std::vector<std::shared_ptr<Entity>>
	EntityManager::RemoveEntitiesNotIn(const std::unordered_set<std::string>& uuidsToKeep)
	{
		std::unique_lock lock(m_MutexEntities);
		std::vector<std::shared_ptr<Entity>> removedEntities;

		for (auto it = m_Entities.begin(); it != m_Entities.end();)
		{
			if (uuidsToKeep.find(it->first) == uuidsToKeep.end())
			{
				removedEntities.push_back(it->second);
				it = m_Entities.erase(it);
			}
			else
			{
				++it;
			}
		}

		return removedEntities;
	}

	std::unordered_map<std::string, std::shared_ptr<Player>> EntityManager::GetAllPlayers() const
	{
		std::shared_lock lock(m_MutexPlayers);
		// Return a copy of the players map to avoid potential issues with concurrent access
		return m_Players;
	}

	std::vector<std::shared_ptr<Entity>> EntityManager::GetAllEntities() const
	{
		std::shared_lock lock(m_MutexEntities);
		std::vector<std::shared_ptr<Entity>> entities;
		entities.reserve(m_Entities.size());
		for (const auto& [uuid, entity] : m_Entities)
		{
			entities.push_back(entity);
		}
		return entities;
	}

	std::vector<std::shared_ptr<Entity>> EntityManager::GetEntitiesInChunk(const glm::ivec2& chunkPosition) const
	{
		std::shared_lock lock(m_MutexEntities);
		std::vector<std::shared_ptr<Entity>> entities;

		for (const auto& [uuid, entity] : m_Entities)
		{
			if (!entity->HasTransform())
			{
				continue;
			}

			if (Utils::WorldToChunkPosition(entity->GetTransform().Position) == chunkPosition)
			{
				entities.push_back(entity);
			}
		}

		return entities;
	}

	std::vector<std::shared_ptr<Entity>> EntityManager::ExtractEntitiesInChunk(const glm::ivec2& chunkPosition)
	{
		std::unique_lock lock(m_MutexEntities);
		std::vector<std::shared_ptr<Entity>> entities;

		for (auto it = m_Entities.begin(); it != m_Entities.end();)
		{
			const auto& entity = it->second;
			if (entity->HasTransform() && Utils::WorldToChunkPosition(entity->GetTransform().Position) == chunkPosition)
			{
				entities.push_back(entity);
				it = m_Entities.erase(it);
			}
			else
			{
				++it;
			}
		}

		return entities;
	}

	void EntityManager::ClearAllEntities()
	{
		// Backups players before clearing so they can be used to trigger events after unlocking
		auto playersRemoved = GetAllPlayers();
		{
			std::unique_lock lock(m_MutexEntities);
			std::unique_lock lockPlayers(m_MutexPlayers);
			m_Players.clear();
			m_Entities.clear();
		}

		for (const auto& [uuid, player] : playersRemoved)
		{
			EvtPlayerRemoved.Trigger(player);
		}
	}

	bool EntityManager::PlayerExists(const std::string& uuid) const
	{
		std::shared_lock lock(m_MutexPlayers);
		return m_Players.find(uuid) != m_Players.end();
	}

} // namespace onion::voxel
