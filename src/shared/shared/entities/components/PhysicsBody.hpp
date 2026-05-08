#pragma once

#include <glm/glm.hpp>

namespace onion::voxel
{
	struct PhysicsBody
	{
		glm::vec3 Velocity{0.f};
		bool OnGround = false;
		bool IsFlying = false;

		float Mass = 1.f;

		// Full extents of the AABB (width, height, depth in world units).
		glm::vec3 Size{1.f};

		// Offset from Transform.Position to the centre of the AABB.
		// e.g. for a player whose origin is at the feet: CenterOffset = (0, Size.y * 0.5, 0)
		glm::vec3 CenterOffset{0.f};
	};

} // namespace onion::voxel
