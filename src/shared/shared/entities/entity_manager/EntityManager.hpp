#pragma once

#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <onion/Event.hpp>

#include <shared/entities/entity/Entity.hpp>
#include <shared/entities/entity/player/Player.hpp>

namespace onion::voxel
{
	class EntityManager
	{
		// ----- Constructor / Destructor -----
	  public:
		EntityManager();
		~EntityManager();

		// ----- Public API -----
	  public:
		std::shared_ptr<Player> GetPlayer(const std::string& uuid) const;
		void AddPlayer(const std::shared_ptr<Player>& player);
		bool RemovePlayer(const std::string& uuid);

		glm::ivec3 GetPlayerPosition(const std::string& uuid) const;
		std::unordered_map<std::string, glm::vec3> GetAllPlayersPosition() const;
		void SetPlayerPosition(const std::string& uuid, const glm::vec3& newPosition);
		void AddEntity(const std::shared_ptr<Entity>& entity);
		void AddOrUpdateEntity(const std::shared_ptr<Entity>& entity);
		std::shared_ptr<Entity> GetEntity(const std::string& uuid) const;
		bool RemoveEntity(const std::string& uuid);
		void UpdateEntities(const std::vector<std::shared_ptr<Entity>>& entities);
		std::vector<std::shared_ptr<Entity>> RemoveEntitiesNotIn(const std::unordered_set<std::string>& uuidsToKeep);
		std::vector<std::shared_ptr<Entity>> GetEntitiesInChunk(const glm::ivec2& chunkPosition) const;
		std::vector<std::shared_ptr<Entity>> ExtractEntitiesInChunk(const glm::ivec2& chunkPosition);

		void ClearAllEntities();

		bool PlayerExists(const std::string& uuid) const;

		std::unordered_map<std::string, std::shared_ptr<Player>> GetAllPlayers() const;

		std::vector<std::shared_ptr<Entity>> GetAllEntities() const;

		// ----- Events -----
	  public:
		Event<const std::shared_ptr<Player>&> EvtPlayerAdded;
		Event<const std::shared_ptr<Player>&> EvtPlayerRemoved;

		// ----- Private Members -----
	  private:
		mutable std::shared_mutex m_MutexPlayers;
		std::unordered_map<std::string, std::shared_ptr<Player>> m_Players;

		mutable std::shared_mutex m_MutexEntities;
		std::unordered_map<std::string, std::shared_ptr<Entity>> m_Entities;
	};
} // namespace onion::voxel
