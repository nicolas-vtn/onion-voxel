#include "PhysicsEngine.hpp"

#include <iostream>

#include <shared/utils/Utils.hpp>

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

	// ---------------------------------------------------------------------------
	// Swept AABB test: moves box `a` by `displacement`, tests against static `b`.
	// Returns the normalised time of first contact in [0, 1) or 1.0 if no hit.
	// `outNormal` is set to the face normal of `b` that was hit (only valid on hit).
	// ---------------------------------------------------------------------------
	float SweptAABB(const AABBWorld& a, const glm::vec3& displacement, const AABBWorld& b, glm::vec3& outNormal)
	{
		// Distances from each face of b to the near/far face of a on each axis
		glm::vec3 invEntry, invExit;

		for (int i = 0; i < 3; ++i)
		{
			if (displacement[i] > 0.0f)
			{
				invEntry[i] = b.Min[i] - a.Max[i];
				invExit[i] = b.Max[i] - a.Min[i];
			}
			else
			{
				invEntry[i] = b.Max[i] - a.Min[i];
				invExit[i] = b.Min[i] - a.Max[i];
			}
		}

		// Times of entry/exit per axis
		glm::vec3 tEntry, tExit;

		for (int i = 0; i < 3; ++i)
		{
			if (displacement[i] == 0.0f)
			{
				// No movement on this axis — use sentinel values
				tEntry[i] = -std::numeric_limits<float>::infinity();
				tExit[i] = std::numeric_limits<float>::infinity();
			}
			else
			{
				tEntry[i] = invEntry[i] / displacement[i];
				tExit[i] = invExit[i] / displacement[i];
			}
		}

		float tEnter = std::max({tEntry.x, tEntry.y, tEntry.z});
		float tLeave = std::min({tExit.x, tExit.y, tExit.z});

		// No collision: separating axis exists, already past, or entry after end of sweep
		if (tEnter >= tLeave || tEnter >= 1.0f || tLeave <= 0.0f)
			return 1.0f;

		// Determine the axis of first contact and build the outward normal from b
		outNormal = glm::vec3(0.0f);
		if (tEntry.x >= tEntry.y && tEntry.x >= tEntry.z)
			outNormal.x = (displacement.x > 0.0f) ? -1.0f : 1.0f;
		else if (tEntry.y >= tEntry.x && tEntry.y >= tEntry.z)
			outNormal.y = (displacement.y > 0.0f) ? -1.0f : 1.0f;
		else
			outNormal.z = (displacement.z > 0.0f) ? -1.0f : 1.0f;

		return tEnter;
	}

} // namespace

namespace onion::voxel
{
	PhysicsEngine::PhysicsEngine(WorldManager& worldManager) : m_WorldManager(worldManager) {}

	PhysicsEngine::~PhysicsEngine() {}

	void PhysicsEngine::Update(float deltaTime)
	{
		// Gets all entities from the WorldManager
		std::vector<std::shared_ptr<Entity>> entities = m_WorldManager.GetAllEntities();

		std::unordered_map<std::string, std::shared_ptr<Player>> players = m_WorldManager.GetAllPlayers();
		for (const auto& [uuid, player] : players)
		{
			entities.push_back(player); // Add players to the list of entities to update physics for
		}

		// Update physics and resolve collisions for each entity
		for (const auto& entity : entities)
		{
			// Check if the entity is in a loaded chunk
			bool inLoadedChunk = false;
			if (entity->HasTransform())
			{
				glm::ivec2 chunkPos = Utils::WorldToChunkPosition(entity->GetTransform().Position);
				inLoadedChunk = m_WorldManager.IsChunkLoaded(chunkPos);
			}

			if (inLoadedChunk)
			{
				UpdateEntityPhysics(entity, deltaTime);
				SweptResolveTerrainCollisions(entity, deltaTime);
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

	void PhysicsEngine::LegacyResolveTerrainCollisions(std::shared_ptr<Entity> entity, float dt)
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
						const BlockState& block = m_WorldManager.GetBlock(glm::ivec3{x, y, z});
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
						const BlockState& block = m_WorldManager.GetBlock(glm::ivec3{x, y, z});
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
						const BlockState& block = m_WorldManager.GetBlock(glm::ivec3{x, y, z});
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

		// Teleport to surface if still colliding after resolution (prevents getting stuck in blocks)
		int safety = 0;
		while (IsCollidingWithTerrain(transform.Position, half, physics.Offset) && safety++ < 20)
		{
			transform.Position.y += 1.0f; // Move up by 1 block until no longer colliding
		}

		entity->SetTransform(transform);
		entity->SetPhysicsBody(physics);
	}

	// -------------------------------------------------------------------------
	// SweptResolveTerrainCollisions
	//
	// Performs up to MAX_ITERATIONS of swept-AABB collision resolution per frame.
	// Each iteration:
	//   1. Broadphase: expand entity AABB by remaining displacement to find candidate blocks.
	//   2. Narrowphase: run SweptAABB against every candidate solid block, find earliest hit.
	//   3. Move entity to the hit point (minus a small epsilon to avoid touching).
	//   4. Zero velocity on the hit normal axis; project remaining displacement onto the
	//      slide plane so the entity slides along walls/floor naturally.
	//   5. Repeat with the remaining displacement until no more hits or iterations exhausted.
	//
	// OnGround is set when the entity lands on a surface whose normal points upward.
	// A small downward ground probe is also run each frame to keep OnGround stable while
	// the entity is standing still on a floor (tEnter == 0 case).
	// -------------------------------------------------------------------------
	void PhysicsEngine::SweptResolveTerrainCollisions(std::shared_ptr<Entity> entity, float dt)
	{
		if (!entity->HasPhysicsBody() || !entity->HasTransform())
			return;

		auto physics = entity->GetPhysicsBody();
		auto transform = entity->GetTransform();

		physics.OnGround = false;

		constexpr int MAX_ITERATIONS = 3;
		constexpr float EPSILON = 0.001f; // Push-back from surface to avoid float overlap

		const glm::vec3 half = physics.HalfSize;
		const glm::vec3 offset = physics.Offset;

		glm::vec3 pos = transform.Position;
		glm::vec3 vel = physics.Velocity;

		// Total displacement this frame
		glm::vec3 remaining = vel * dt;

		for (int iter = 0; iter < MAX_ITERATIONS; ++iter)
		{
			if (glm::length(remaining) < EPSILON * 0.1f)
				break;

			// --- Broadphase: AABB swept over the remaining displacement ---
			glm::vec3 center = pos + offset;
			AABBWorld swept;
			swept.Min = glm::min(center - half, center - half + remaining) - glm::vec3(EPSILON);
			swept.Max = glm::max(center + half, center + half + remaining) + glm::vec3(EPSILON);

			int bMinX = static_cast<int>(std::floor(swept.Min.x));
			int bMaxX = static_cast<int>(std::floor(swept.Max.x));
			int bMinY = static_cast<int>(std::floor(swept.Min.y));
			int bMaxY = static_cast<int>(std::floor(swept.Max.y));
			int bMinZ = static_cast<int>(std::floor(swept.Min.z));
			int bMaxZ = static_cast<int>(std::floor(swept.Max.z));

			// --- Narrowphase: find the earliest hit among all candidate blocks ---
			float tMin = 1.0f;
			glm::vec3 hitNormal = glm::vec3(0.0f);

			AABBWorld entityBox{center - half, center + half};

			for (int bx = bMinX; bx <= bMaxX; ++bx)
			{
				for (int by = bMinY; by <= bMaxY; ++by)
				{
					for (int bz = bMinZ; bz <= bMaxZ; ++bz)
					{
						const BlockState& block = m_WorldManager.GetBlock(glm::ivec3{bx, by, bz});
						if (!BlockState::IsSolid(block.ID))
							continue;

						// Full-cube AABB for this block
						AABBWorld blockBox{glm::vec3(bx, by, bz), glm::vec3(bx + 1.0f, by + 1.0f, bz + 1.0f)};

						glm::vec3 normal;
						float t = SweptAABB(entityBox, remaining, blockBox, normal);

						if (t < tMin)
						{
							tMin = t;
							hitNormal = normal;
						}
					}
				}
			}

			if (tMin >= 1.0f)
			{
				// No collision this iteration — consume all remaining displacement
				pos += remaining;
				break;
			}

			// Move entity to the point of contact, leaving a tiny gap.
			// Convert the surface epsilon to a time offset based on displacement length.
			float dispLen = glm::length(remaining);
			float tEpsilon = (dispLen > 0.0f) ? (EPSILON / dispLen) : 0.0f;
			float tSafe = std::max(0.0f, tMin - tEpsilon);
			pos += remaining * tSafe;

			// Detect ground contact (normal pointing upward)
			if (hitNormal.y > 0.5f)
			{
				physics.OnGround = true;
				vel.y = 0.0f;
			}
			// Ceiling hit
			else if (hitNormal.y < -0.5f)
			{
				vel.y = 0.0f;
			}
			// Wall hit on X
			if (std::abs(hitNormal.x) > 0.5f)
				vel.x = 0.0f;

			// Wall hit on Z
			if (std::abs(hitNormal.z) > 0.5f)
				vel.z = 0.0f;

			// Remaining displacement: subtract the consumed portion, then project onto
			// the slide plane (remove the component along the hit normal).
			remaining = remaining * (1.0f - tSafe);
			remaining -= glm::dot(remaining, hitNormal) * hitNormal;
		}

		// ---- Ground probe ----
		// When the entity is resting on a floor it may have zero vertical velocity, so
		// the sweep above won't detect a hit. Cast a tiny ray downward to check.
		if (!physics.OnGround && !physics.IsFlying)
		{
			glm::vec3 center = pos + offset;
			AABBWorld probeBox{center - half, center + half};
			glm::vec3 probeDisp(0.0f, -EPSILON * 4.0f, 0.0f);

			glm::vec3 pCenter = pos + offset;
			int bMinX = static_cast<int>(std::floor(pCenter.x - half.x));
			int bMaxX = static_cast<int>(std::floor(pCenter.x + half.x));
			int bY = static_cast<int>(std::floor(pCenter.y - half.y - EPSILON * 2.0f));
			int bMinZ = static_cast<int>(std::floor(pCenter.z - half.z));
			int bMaxZ = static_cast<int>(std::floor(pCenter.z + half.z));

			for (int bx = bMinX; bx <= bMaxX && !physics.OnGround; ++bx)
			{
				for (int bz = bMinZ; bz <= bMaxZ && !physics.OnGround; ++bz)
				{
					const BlockState& block = m_WorldManager.GetBlock(glm::ivec3{bx, bY, bz});
					if (!BlockState::IsSolid(block.ID))
						continue;

					AABBWorld blockBox{glm::vec3(bx, bY, bz), glm::vec3(bx + 1.0f, bY + 1.0f, bz + 1.0f)};
					glm::vec3 normal;
					float t = SweptAABB(probeBox, probeDisp, blockBox, normal);
					if (t < 1.0f && normal.y > 0.5f)
						physics.OnGround = true;
				}
			}
		}

		// ---- Apply ----
		transform.Position = pos;
		physics.Velocity = vel;

		entity->SetTransform(transform);
		entity->SetPhysicsBody(physics);
	}

	bool
	PhysicsEngine::IsCollidingWithTerrain(const glm::vec3& position, const glm::vec3& halfSize, const glm::vec3& offset)
	{
		glm::vec3 center = position + offset;
		glm::vec3 min = center - halfSize;
		glm::vec3 max = center + halfSize;

		int minX = static_cast<int>(std::floor(min.x));
		int maxX = static_cast<int>(std::floor(max.x));
		int minY = static_cast<int>(std::floor(min.y));
		int maxY = static_cast<int>(std::floor(max.y));
		int minZ = static_cast<int>(std::floor(min.z));
		int maxZ = static_cast<int>(std::floor(max.z));

		for (int x = minX; x <= maxX; ++x)
		{
			for (int y = minY; y <= maxY; ++y)
			{
				for (int z = minZ; z <= maxZ; ++z)
				{
					const BlockState& block = m_WorldManager.GetBlock(glm::ivec3{x, y, z});
					if (!BlockState::IsSolid(block.ID))
						continue;

					// AABB vs block test (block = [x,x+1])
					if (max.x > x && min.x < x + 1.0f && max.y > y && min.y < y + 1.0f && max.z > z && min.z < z + 1.0f)
					{
						return true;
					}
				}
			}
		}

		return false;
	}

} // namespace onion::voxel
