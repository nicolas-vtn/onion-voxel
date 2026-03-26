#include "EntityManager.hpp"

#include <stdexcept>

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
		std::unique_lock lock(m_MutexPlayers);
		// Throw an exception if a player with the same UUID already exists
		if (m_Players.find(player->UUID) != m_Players.end())
		{
			throw std::runtime_error("Player with UUID " + player->UUID + " already exists.");
		}

		m_Players[player->UUID] = player;
	}

	bool EntityManager::RemovePlayer(const std::string& uuid)
	{
		std::unique_lock lock(m_MutexPlayers);
		auto it = m_Players.find(uuid);
		if (it != m_Players.end())
		{
			// Remove the player from the entities list
			m_Players.erase(it);
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
				auto it = std::find_if(m_Entities.begin(),
									   m_Entities.end(),
									   [&entity](const std::shared_ptr<Entity>& e) { return e->UUID == entity->UUID; });

				if (it != m_Entities.end())
				{
					*it = entity; // Update existing entity
				}
				else
				{
					m_Entities.push_back(entity); // Add new entity
				}
			}
		}
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
		// Return a copy of the entities vector to avoid potential issues with concurrent access
		return m_Entities;
	}

	void EntityManager::ClearAllEntities()
	{
		std::unique_lock lockPlayers(m_MutexPlayers);
		std::unique_lock lockEntities(m_MutexEntities);
		m_Players.clear();
		m_Entities.clear();
	}

	bool EntityManager::IsPlayerExists(const std::string& uuid) const
	{
		std::shared_lock lock(m_MutexPlayers);
		return m_Players.find(uuid) != m_Players.end();
	}

	void EntityManager::SetLocalPlayer(const std::shared_ptr<Player>& player)
	{
		std::unique_lock lock(m_MutexLocalPlayer);
		m_LocalPlayer = player;
	}

	std::shared_ptr<Player> EntityManager::GetLocalPlayer() const
	{
		std::shared_lock lock(m_MutexLocalPlayer);
		return m_LocalPlayer;
	}

} // namespace onion::voxel
