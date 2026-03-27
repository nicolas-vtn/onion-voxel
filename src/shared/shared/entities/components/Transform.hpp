#pragma once

#include <glm/glm.hpp>

namespace onion::voxel
{
	struct Transform
	{
		glm::vec3 Position{0.f};
		glm::vec3 Rotation{0.f};
		glm::vec3 Scale{1.f};
	};

} // namespace onion::voxel
