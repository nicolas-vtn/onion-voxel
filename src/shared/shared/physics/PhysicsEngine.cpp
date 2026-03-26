#include "PhysicsEngine.hpp"

namespace onion::voxel
{
	PhysicsEngine::PhysicsEngine(EntityManager& entityManager) : m_entityManager(entityManager) {}

	PhysicsEngine::~PhysicsEngine() {}

	void PhysicsEngine::Update(float deltaTime)
	{
		float gravity = GetGravity();

		// Gets all entities from the EntityManager
		std::vector<std::shared_ptr<Entity>> entities = m_entityManager.GetAllEntities();

		std::unordered_map<std::string, std::shared_ptr<Player>> players = m_entityManager.GetAllPlayers();
		for (const auto& [uuid, player] : players)
		{
			entities.push_back(player); // Add players to the list of entities to update physics for
		}

		// Update physics for each entity
		for (const auto& entity : entities)
		{
			if (entity->HasPhysicsBody() && entity->HasTransform())
			{
				PhysicsBody physicsBody = entity->GetPhysicsBody();
				Transform transform = entity->GetTransform();

				// Apply gravity if enabled
				if (physicsBody.EnableGravity && !physicsBody.OnGround)
				{
					physicsBody.Velocity.y -= gravity * deltaTime; // Simple gravity
				}

				// Update position based on velocity
				transform.Position += physicsBody.Velocity * deltaTime;

				// Here you would typically check for collisions and adjust the position and velocity accordingly
				// Update the entity's components
				entity->SetPhysicsBody(physicsBody);
				entity->SetTransform(transform);
			}
		}
	}

	float PhysicsEngine::GetGravity() const
	{
		std::shared_lock lock(m_MutexPhysics);
		return m_Gravity;
	}

	void PhysicsEngine::SetGravity(float gravity)
	{
		std::unique_lock lock(m_MutexPhysics);
		m_Gravity = gravity;
	}

} // namespace onion::voxel
