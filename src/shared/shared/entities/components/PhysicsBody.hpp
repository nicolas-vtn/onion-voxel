#pragma once

#include <glm/glm.hpp>

namespace onion::voxel
{
	struct PhysicsBody
	{
		glm::vec3 Velocity{0.f};
		bool OnGround = false;
		bool EnableGravity = true;

		float Mass = 1.f;

		glm::vec3 HalfSize{1.f};
		glm::vec3 Offset{0.f};
	};

} // namespace onion::voxel
