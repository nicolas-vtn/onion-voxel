#include "PhysicsEngine.hpp"

#include <iostream>

#include <shared/utils/Utils.hpp>
#include <shared/world/block/BlockstateRegistry.hpp>

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
				// No movement on this axis — collision is only possible if already overlapping spatially.
				// Using (-inf, +inf) sentinels unconditionally causes false hits on blocks that are
				// not overlapping on this axis (e.g. a ceiling block when sweeping horizontally).
				if (a.Max[i] <= b.Min[i] || a.Min[i] >= b.Max[i])
					return 1.0f; // no spatial overlap on stationary axis — no hit possible
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

	// -------------------------------------------------------------------------
	// GetBlockElementAABBs
	//
	// Returns the world-space AABBs for each BlockModel::Element of a solid block.
	// Element From/To are in Minecraft 0–16 units; dividing by 16 and offsetting by
	// the block's integer world position converts them to world-space metres.
	//
	// Element rotation (ElementRotation) is intentionally ignored: collision-relevant
	// blocks (slabs, stairs, fences, snow) never use element rotation. For any element
	// that does have a non-zero rotation angle we still use its From/To AABB directly —
	// this is slightly conservative but never under-estimates the actual geometry.
	//
	// Falls back to a single full-cube AABB if the block has no model or no elements.
	// -------------------------------------------------------------------------
	std::vector<AABBWorld> GetBlockElementAABBs(const BlockState& state, const glm::ivec3& blockPos)
	{
		const glm::vec3 origin = glm::vec3(blockPos);

		const auto& registry = BlockstateRegistry::Get();
		auto it = registry.find(state.ID);

		if (it != registry.end())
		{
			const std::vector<BlockModel::Element>& elements = it->second[state.VariantIndex].Model.Elements;

			if (!elements.empty())
			{
				std::vector<AABBWorld> result;
				result.reserve(elements.size());

				for (const auto& element : elements)
				{
					// Convert from Minecraft 0–16 units to world-space metres
					AABBWorld box;
					box.Min = origin + element.From / 16.0f;
					box.Max = origin + element.To / 16.0f;
					result.push_back(box);
				}

				return result;
			}
		}

		// Fallback: full unit cube
		return {AABBWorld{origin, origin + glm::vec3(1.0f)}};
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
		while (LegacyIsCollidingWithTerrain(transform.Position, half, physics.Offset) && safety++ < 20)
		{
			transform.Position.y += 1.0f; // Move up by 1 block until no longer colliding
		}

		entity->SetTransform(transform);
		entity->SetPhysicsBody(physics);
	}

	// -------------------------------------------------------------------------
	// SweptResolveTerrainCollisions
	//
	// Resolves terrain collisions using per-axis swept AABB in order Y → X → Z.
	// Sweeping each axis independently eliminates the priority ambiguity that occurs
	// when a floor and a wall are both hit in the same frame (a combined 3D sweep can
	// pick the wrong normal due to float noise, causing stickiness).
	//
	// For each axis:
	//   1. Build a 1D displacement vector for that axis only.
	//   2. Broadphase: expand entity AABB along that displacement to get candidate blocks.
	//   3. Narrowphase: find the earliest SweptAABB hit (t in [0,1)).
	//   4. Move entity to t_safe = (tMin - epsilon/|disp|), clamped to [0,1].
	//   5. Zero velocity on that axis if a hit occurred.
	//
	// OnGround is set when a downward Y sweep hits a floor (normal.y > 0).
	// A ground probe is run when vel.y == 0 so OnGround stays set while standing still.
	// -------------------------------------------------------------------------
	void PhysicsEngine::SweptResolveTerrainCollisions(std::shared_ptr<Entity> entity, float dt)
	{
		if (!entity->HasPhysicsBody() || !entity->HasTransform())
			return;

		auto physics = entity->GetPhysicsBody();
		auto transform = entity->GetTransform();

		bool wasOnGround = physics.OnGround;

		physics.OnGround = false;

		constexpr float EPSILON = 0.001f;

		const glm::vec3 half = physics.HalfSize;
		const glm::vec3 offset = physics.Offset;

		glm::vec3 pos = transform.Position;
		glm::vec3 vel = physics.Velocity;

		// ------------------------------------------------------------------
		// Per-axis sweep helper: sweeps the entity AABB along `disp` (which
		// has only one non-zero component), finds the earliest solid block
		// hit, moves pos to the safe contact point, and zeroes the velocity
		// component on the hit axis. Returns the hit normal (zero if no hit).
		// ------------------------------------------------------------------
		auto sweepAxis = [&](glm::vec3 disp) -> glm::vec3
		{
			float dispLen = std::abs(disp.x) + std::abs(disp.y) + std::abs(disp.z); // only one axis non-zero
			if (dispLen < 1e-6f)
				return glm::vec3(0.0f);

			glm::vec3 center = pos + offset;
			AABBWorld entityBox{center - half, center + half};

			// Broadphase: swept AABB bounds
			AABBWorld swept;
			swept.Min = glm::min(entityBox.Min, entityBox.Min + disp) - glm::vec3(EPSILON);
			swept.Max = glm::max(entityBox.Max, entityBox.Max + disp) + glm::vec3(EPSILON);

			int bMinX = static_cast<int>(std::floor(swept.Min.x));
			int bMaxX = static_cast<int>(std::floor(swept.Max.x));
			int bMinY = static_cast<int>(std::floor(swept.Min.y));
			int bMaxY = static_cast<int>(std::floor(swept.Max.y));
			int bMinZ = static_cast<int>(std::floor(swept.Min.z));
			int bMaxZ = static_cast<int>(std::floor(swept.Max.z));

			float tMin = 1.0f;
			glm::vec3 hitNormal = glm::vec3(0.0f);

			for (int bx = bMinX; bx <= bMaxX; ++bx)
				for (int by = bMinY; by <= bMaxY; ++by)
					for (int bz = bMinZ; bz <= bMaxZ; ++bz)
					{
						const BlockState& block = m_WorldManager.GetBlock(glm::ivec3{bx, by, bz});
						if (!BlockState::IsSolid(block.ID))
							continue;

						// Test each element AABB of the block model individually.
						// Solid blocks with no model fall back to a full unit cube.
						for (const AABBWorld& elemBox : GetBlockElementAABBs(block, {bx, by, bz}))
						{
							glm::vec3 normal;
							float t = SweptAABB(entityBox, disp, elemBox, normal);
							if (t < tMin)
							{
								tMin = t;
								hitNormal = normal;
							}
						}
					}

			if (tMin < 1.0f)
			{
				// Move to safe contact point: stop epsilon before the surface
				float tEpsilon = EPSILON / dispLen;
				float tSafe = std::max(0.0f, tMin - tEpsilon);
				pos += disp * tSafe;

				// Zero velocity on the hit axis. Per-axis sweeping makes this safe:
				// - vel.y must be zeroed to prevent gravity accumulation through floors/ceilings.
				// - vel.x/z zeroing on wall hits no longer causes stickiness because each axis
				//   is swept independently — the floor cannot interfere with an X wall hit.
				//   Zeroing also prevents velocity stacking when running against a wall.
				if (std::abs(hitNormal.x) > 0.5f)
					vel.x = 0.0f;
				if (std::abs(hitNormal.y) > 0.5f)
					vel.y = 0.0f;
				if (std::abs(hitNormal.z) > 0.5f)
					vel.z = 0.0f;
			}
			else
			{
				// No hit — consume full displacement
				pos += disp;
			}

			return hitNormal;
		};

		// ------------------------------------------------------------------
		// Checks if player is trying to move stairs / slab / ...
		// Idea : Apply all movements, check collisions. If colliding, move up a bit,
		// check collisions again, repeat until no collision or max step height reached.
		// ------------------------------------------------------------------
		{
			constexpr float MAX_STEP_HEIGHT = 0.6f; // Maximum height the player can step up (e.g., stairs or slabs)
			glm::vec3 playerPos = pos + vel * dt;

			constexpr float STEP_EPSILON = 0.01f; // Small value to prevent floating-point issues

			if (wasOnGround && IsCollidingWithTerrain(playerPos, half, offset))
			{
				float stepHeight = 0.0f;
				while (stepHeight < MAX_STEP_HEIGHT && IsCollidingWithTerrain(playerPos, half, offset))
				{
					stepHeight += STEP_EPSILON;
					playerPos.y += STEP_EPSILON;
				}

				if (stepHeight < MAX_STEP_HEIGHT)
				{
					pos.y += stepHeight;
				}
			}
		}

		// ------------------------------------------------------------------
		// Axis order: Y first (gravity), then X, then Z.
		// Resolving Y first ensures the floor normal is already handled before
		// horizontal sweeps run, so horizontal movement is never eaten by the
		// floor collision.
		// ------------------------------------------------------------------

		// Y axis
		{
			glm::vec3 normal = sweepAxis(glm::vec3(0.0f, vel.y * dt, 0.0f));
			if (normal.y > 0.5f)
				physics.OnGround = true;
		}

		// X axis
		sweepAxis(glm::vec3(vel.x * dt, 0.0f, 0.0f));

		// Z axis
		sweepAxis(glm::vec3(0.0f, 0.0f, vel.z * dt));

		// ------------------------------------------------------------------
		// Ground probe — when vel.y is zero (standing still or just landed),
		// the Y sweep above has zero displacement and won't detect the floor.
		// Cast a tiny downward displacement to confirm ground contact.
		// The probe displacement is small enough to not visibly move the entity.
		// ------------------------------------------------------------------
		if (!physics.OnGround && !physics.IsFlying)
		{
			float preProbeY = pos.y;
			glm::vec3 probeNormal = sweepAxis(glm::vec3(0.0f, -EPSILON * 4.0f, 0.0f));
			if (probeNormal.y > 0.5f)
			{
				physics.OnGround = true;
				vel.y = 0.0f;
			}
			// Always restore Y — the probe must not physically move the entity
			pos.y = preProbeY;
		}

		// ------------------------------------------------------------------
		// Apply
		// ------------------------------------------------------------------
		transform.Position = pos;
		physics.Velocity = vel;

		entity->SetTransform(transform);
		entity->SetPhysicsBody(physics);
	}

	bool PhysicsEngine::LegacyIsCollidingWithTerrain(const glm::vec3& position,
													 const glm::vec3& halfSize,
													 const glm::vec3& offset)
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
			for (int y = minY; y <= maxY; ++y)
				for (int z = minZ; z <= maxZ; ++z)
				{
					const BlockState& block = m_WorldManager.GetBlock(glm::ivec3{x, y, z});
					if (!BlockState::IsSolid(block.ID))
						continue;

					const auto& boxes = GetBlockElementAABBs(block, {x, y, z});
					for (const AABBWorld& elemBox : boxes)
					{
						if (max.x > elemBox.Min.x && min.x < elemBox.Max.x && max.y > elemBox.Min.y &&
							min.y < elemBox.Max.y && max.z > elemBox.Min.z && min.z < elemBox.Max.z)
						{
							return true;
						}
					}
				}

		return false;
	}

	// -------------------------------------------------------------------------
	// HasGroundSupport
	//
	// Returns true when there is at least one solid block directly beneath the
	// player AABB at the given position. The probe checks a 1-block-tall slice
	// just below the feet of the AABB, which is sufficient to determine whether
	// the player is standing on (or would be standing on) solid ground.
	//
	// Used by sneak edge-prevention: before applying horizontal velocity we test
	// whether the candidate position would leave the player unsupported, and if
	// so we discard that velocity component so the player stays on the edge.
	// -------------------------------------------------------------------------
	bool
	PhysicsEngine::HasGroundSupport(const glm::vec3& position, const glm::vec3& halfSize, const glm::vec3& offset) const
	{
		// The feet centre is at (position + offset) - (0, halfSize.y, 0).
		// We probe a thin horizontal slab just below that point.
		constexpr float probeEpsilon = 0.51f; // how far below feet we look

		glm::vec3 center = position + offset;
		glm::vec3 feetMin = {center.x - halfSize.x, center.y - halfSize.y - probeEpsilon, center.z - halfSize.z};
		glm::vec3 feetMax = {center.x + halfSize.x, center.y - halfSize.y, center.z + halfSize.z};

		int minX = static_cast<int>(std::floor(feetMin.x));
		int maxX = static_cast<int>(std::floor(feetMax.x));
		int minY = static_cast<int>(std::floor(feetMin.y));
		int maxY = static_cast<int>(std::floor(feetMax.y));
		int minZ = static_cast<int>(std::floor(feetMin.z));
		int maxZ = static_cast<int>(std::floor(feetMax.z));

		for (int x = minX; x <= maxX; ++x)
			for (int y = minY; y <= maxY; ++y)
				for (int z = minZ; z <= maxZ; ++z)
				{
					const BlockState& block = m_WorldManager.GetBlock(glm::ivec3{x, y, z});
					if (!BlockState::IsSolid(block.ID))
						continue;

					const auto& boxes = GetBlockElementAABBs(block, {x, y, z});
					for (const AABBWorld& elemBox : boxes)
					{
						if (feetMax.x > elemBox.Min.x && feetMin.x < elemBox.Max.x && feetMax.y > elemBox.Min.y &&
							feetMin.y < elemBox.Max.y && feetMax.z > elemBox.Min.z && feetMin.z < elemBox.Max.z)
						{
							return true;
						}
					}
				}

		return false;
	}

} // namespace onion::voxel
