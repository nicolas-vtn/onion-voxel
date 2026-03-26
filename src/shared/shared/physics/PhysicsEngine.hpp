#pragma once

#include <shared_mutex>

#include <shared/entities/entity_manager/EntityManager.hpp>

namespace onion::voxel
{
	class PhysicsEngine
	{

		// ----- Constructor / Destructor -----
	  public:
		PhysicsEngine(EntityManager& entityManager);
		~PhysicsEngine();

		// ----- Public API -----
	  public:
		void Update(float deltaTime);

		// ----- Getters / Setters -----
	  public:
		float GetGravity() const;
		void SetGravity(float gravity);

		// ----- Private Members -----
	  private:
		EntityManager& m_entityManager;

		// ----- Private Constants -----
	  private:
		mutable std::shared_mutex m_MutexPhysics;

		float m_Gravity = -9.81f; // Gravity acceleration in m/s^2
	};
} // namespace onion::voxel
