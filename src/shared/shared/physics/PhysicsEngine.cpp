#include "PhysicsEngine.hpp"

#include <iostream>

namespace
{
	using namespace onion::voxel;

	struct AABBWorld
	{
		glm::vec3 Min;
		glm::vec3 Max;
	};

	AABBWorld ComputeAABB(const Transform& t, const PhysicsBody& p)
	{
		glm::vec3 center = t.Position + p.Offset;

		return {center - p.HalfSize, center + p.HalfSize};
	}
} // namespace

namespace onion::voxel
{
	PhysicsEngine::PhysicsEngine(WorldManager& worldManager)
		: m_WorldManager(worldManager), m_EntityManager(*worldManager.Entities)
	{
	}

	PhysicsEngine::~PhysicsEngine() {}

	void PhysicsEngine::Update(float deltaTime)
	{
		// Gets all entities from the EntityManager
		std::vector<std::shared_ptr<Entity>> entities = m_EntityManager.GetAllEntities();

		std::unordered_map<std::string, std::shared_ptr<Player>> players = m_EntityManager.GetAllPlayers();
		for (const auto& [uuid, player] : players)
		{
			entities.push_back(player); // Add players to the list of entities to update physics for
		}

		// Update physics and resolve collisions for each entity
		for (const auto& entity : entities)
		{
			UpdateEntityPhysics(entity, deltaTime);
			ResolveTerrainCollisions(entity, deltaTime);
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

	float PhysicsEngine::GetJumpStrength() const
	{
		std::shared_lock lock(m_MutexPhysics);
		return m_JumpStrength;
	}

	void PhysicsEngine::SetJumpStrength(float jumpStrength)
	{
		std::unique_lock lock(m_MutexPhysics);
		m_JumpStrength = jumpStrength;
	}

	void PhysicsEngine::UpdateEntityPhysics(std::shared_ptr<Entity> entity, float deltaTime)
	{
		if (entity->HasPhysicsBody() && entity->HasTransform())
		{
			PhysicsBody physicsBody = entity->GetPhysicsBody();

			// Apply gravity if not flying and not on the ground
			if (!physicsBody.IsFlying && !physicsBody.OnGround)
			{
				physicsBody.Velocity.y -= GetGravity() * deltaTime;
			}

			// Apply friction if on the ground and not flying
			if (!physicsBody.IsFlying && physicsBody.OnGround)
			{
				ApplyFriction(physicsBody.Velocity, deltaTime);
			}

			// Update the entity's components
			entity->SetPhysicsBody(physicsBody);
		}
	}

	void PhysicsEngine::ApplyFriction(glm::vec3& velocity, float deltaTime)
	{

		const float epsilon = 0.0001f; // Small value to prevent floating-point issues

		float speedX = std::abs(velocity.x);
		if (speedX < epsilon)
			velocity.x = 0.0f;

		float speedZ = std::abs(velocity.z);
		if (speedZ < epsilon)
			velocity.z = 0.0f;

		float k = 2.0f; // Tuning parameter for friction strength

		float frictionAmountX = (m_GroundFriction + k * speedX) * deltaTime;
		float frictionAmountZ = (m_GroundFriction + k * speedZ) * deltaTime;

		//std::cout << "frictionAmountX: " << frictionAmountX << ", frictionAmountZ: " << frictionAmountZ << std::endl;

		velocity.x -= std::min(speedX, frictionAmountX) * glm::sign(velocity.x);
		velocity.z -= std::min(speedZ, frictionAmountZ) * glm::sign(velocity.z);
	}

	void PhysicsEngine::ResolveTerrainCollisions(std::shared_ptr<Entity> entity, float dt)
	{
		if (!entity->HasPhysicsBody() || !entity->HasTransform())
			return;

		auto physics = entity->GetPhysicsBody();
		auto transform = entity->GetTransform();

		physics.OnGround = false;

		glm::vec3 pos = transform.Position;
		glm::vec3 vel = physics.Velocity;
		glm::vec3 half = physics.HalfSize;

		constexpr float epsilon = 0.0001f; // Small value to prevent floating-point issues

		// ---- AXIS RESOLUTION ----

		// === Y AXIS (vertical first = important for gravity) ===
		pos.y += vel.y * dt; // Move the entity according to its velocity before checking for collisions
		{
			glm::vec3 center = pos + physics.Offset;
			AABBWorld box{center - half, center + half};

			int minX = (int) std::floor(box.Min.x);
			int maxX = (int) std::floor(box.Max.x);
			int minY = (int) std::floor(box.Min.y);
			int maxY = (int) std::floor(box.Max.y);
			int minZ = (int) std::floor(box.Min.z);
			int maxZ = (int) std::floor(box.Max.z);

			for (int x = minX; x <= maxX; x++)
				for (int y = minY; y <= maxY; y++)
					for (int z = minZ; z <= maxZ; z++)
					{
						const BlockState& block = m_WorldManager.GetBlock({x, y, z});
						if (!BlockState::IsSolid(block.ID))
							continue;

						float blockMinY = static_cast<float>(y);
						float blockMaxY = y + 1.0f;

						// Falling
						if (vel.y < 0.0f)
						{
							float penetration = box.Min.y - blockMaxY;
							if (penetration < 0.0f)
							{
								//std::cout << "Collision detected on Y axis - Penetration: " << penetration << std::endl;
								pos.y -= penetration;
								vel.y = 0.0f;
								physics.OnGround = true;
							}
						}
						// Jumping
						else if (vel.y > 0.0f)
						{
							float penetration = box.Max.y - blockMinY;
							if (penetration > 0.0f)
							{
								//std::cout << "Collision detected on Y axis - Penetration: " << penetration << std::endl;
								pos.y -= penetration;
								vel.y = 0.0f;
							}
						}
					}
		}

		// === X AXIS ===
		pos.x += vel.x * dt;
		{
			glm::vec3 center = pos + physics.Offset;
			AABBWorld box{center - half, center + half};

			int minX = (int) std::floor(box.Min.x);
			int maxX = (int) std::floor(box.Max.x);
			int minY = (int) std::floor(box.Min.y);
			int maxY = (int) std::floor(box.Max.y);
			int minZ = (int) std::floor(box.Min.z);
			int maxZ = (int) std::floor(box.Max.z);

			for (int x = minX; x <= maxX; x++)
				for (int y = minY; y <= maxY; y++)
					for (int z = minZ; z <= maxZ; z++)
					{
						const BlockState& block = m_WorldManager.GetBlock({x, y, z});
						if (!BlockState::IsSolid(block.ID))
							continue;

						if (vel.x > 0.0f)
						{
							float penetration = box.Max.x - x;
							if (penetration > 0.0f)
							{
								//std::cout << "Collision detected on X axis - Penetration: " << penetration << std::endl;
								pos.x -= penetration + glm::sign(penetration) * epsilon;
								vel.x = 0.0f;
							}
						}
						else if (vel.x < 0.0f)
						{
							float penetration = box.Min.x - (x + 1.0f);
							if (penetration < 0.0f)
							{
								//std::cout << "Collision detected on X axis - Penetration: " << penetration << std::endl;
								pos.x -= penetration + glm::sign(penetration) * epsilon;
								vel.x = 0.0f;
							}
						}
					}
		}

		// === Z AXIS ===
		pos.z += vel.z * dt;
		{
			glm::vec3 center = pos + physics.Offset;
			AABBWorld box{center - half, center + half};

			int minX = (int) std::floor(box.Min.x);
			int maxX = (int) std::floor(box.Max.x);
			int minY = (int) std::floor(box.Min.y);
			int maxY = (int) std::floor(box.Max.y);
			int minZ = (int) std::floor(box.Min.z);
			int maxZ = (int) std::floor(box.Max.z);

			for (int x = minX; x <= maxX; x++)
				for (int y = minY; y <= maxY; y++)
					for (int z = minZ; z <= maxZ; z++)
					{
						const BlockState& block = m_WorldManager.GetBlock({x, y, z});
						if (!BlockState::IsSolid(block.ID))
							continue;

						if (vel.z > 0.0f)
						{
							float penetration = box.Max.z - z;
							if (penetration > 0.0f)
							{
								//std::cout << "Collision detected on Z axis - Penetration: " << penetration << std::endl;
								pos.z -= penetration + glm::sign(penetration) * epsilon;
								vel.z = 0.0f;
							}
						}
						else if (vel.z < 0.0f)
						{
							float penetration = box.Min.z - (z + 1.0f);
							if (penetration < 0.0f)
							{
								//std::cout << "Collision detected on Z axis - Penetration: " << penetration << std::endl;
								pos.z -= penetration + glm::sign(penetration) * epsilon;
								vel.z = 0.0f;
							}
						}
					}
		}

		// ---- APPLY ----
		transform.Position = pos;
		physics.Velocity = vel;

		entity->SetTransform(transform);
		entity->SetPhysicsBody(physics);
	}

} // namespace onion::voxel
