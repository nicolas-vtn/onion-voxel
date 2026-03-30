#pragma once

#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

#include <shared/entities/entity/Entity.hpp>
#include <shared/entities/entity/player/Player.hpp>

namespace onion::voxel
{
	class EntityManager
	{
	  public:
		EntityManager();
		~EntityManager();

		std::shared_ptr<Player> GetPlayer(const std::string& uuid) const;
		void AddPlayer(const std::shared_ptr<Player>& player);
		bool RemovePlayer(const std::string& uuid);

		glm::ivec3 GetPlayerPosition(const std::string& uuid) const;
		std::unordered_map<std::string, glm::vec3> GetAllPlayersPosition() const;
		void SetPlayerPosition(const std::string& uuid, const glm::vec3& newPosition);
		void UpdateEntities(const std::vector<std::shared_ptr<Entity>>& entities);

		void ClearAllEntities();

		bool IsPlayerExists(const std::string& uuid) const;

		std::unordered_map<std::string, std::shared_ptr<Player>> GetAllPlayers() const;

		std::vector<std::shared_ptr<Entity>> GetAllEntities() const;

	  private:
		mutable std::shared_mutex m_MutexPlayers;
		std::unordered_map<std::string, std::shared_ptr<Player>> m_Players;

		mutable std::shared_mutex m_MutexEntities;
		std::vector<std::shared_ptr<Entity>> m_Entities;
	};
} // namespace onion::voxel
