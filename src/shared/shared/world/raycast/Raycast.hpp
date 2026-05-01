#pragma once

#include <glm/glm.hpp>

#include <optional>

#include <shared/world/world_manager/WorldManager.hpp>

namespace onion::voxel
{
	struct RaycastHit
	{
		glm::vec3 HitPosition{0.f};
		Block HitBlock;
		Block AdjacentBlock;
		glm::ivec3 HitFaceNormal{0}; // Direction from hit block toward adjacent block (e.g. (0,1,0) = top face)
	};

	class Raycaster
	{
	  public:
		static std::optional<RaycastHit> Raycast(const WorldManager& worldManager,
												 const glm::vec3& origin,
												 const glm::vec3& direction,
												 float maxDistance,
												 int steps);

		static std::optional<RaycastHit>
		Raycast(const WorldManager& worldManager, const glm::vec3& origin, const glm::vec3& target, float steps);
	};
} // namespace onion::voxel
