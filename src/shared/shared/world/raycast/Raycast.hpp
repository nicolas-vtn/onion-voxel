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
