#pragma once

#include <shared_mutex>

#include <shared/entities/entity_manager/EntityManager.hpp>
#include <shared/world/world_manager/WorldManager.hpp>

namespace onion::voxel
{
	class PhysicsEngine
	{

		// ----- Constructor / Destructor -----
	  public:
		PhysicsEngine(WorldManager& worldManager);
		~PhysicsEngine();

		// ----- Public API -----
	  public:
		void Update(float deltaTime);

		// ----- Getters / Setters -----
	  public:
		float GetGravity() const;
		void SetGravity(float gravity);

		float GetJumpStrength() const;
		void SetJumpStrength(float jumpStrength);

		// ----- Private Members -----
	  private:
		WorldManager& m_WorldManager;

		// ----- Internal Methods -----
	  private:
		void UpdateEntityPhysics(std::shared_ptr<Entity> entity, float deltaTime);
		void ApplyFriction(glm::vec3& velocity, float deltaTime);
		void ResolveTerrainCollisions(std::shared_ptr<Entity> entity, float deltaTime);

		bool IsCollidingWithTerrain(const glm::vec3& position, const glm::vec3& halfSize, const glm::vec3& offset);

		// ----- Private Constants -----
	  private:
		mutable std::shared_mutex m_MutexPhysics;

		float m_Gravity = 50.0f;	  // Gravity acceleration in m/s^2
		float m_JumpStrength = 13.0f; // Initial jump velocity

		float m_GroundFriction = 10.0f; // Friction applied when on the ground
	};
} // namespace onion::voxel
