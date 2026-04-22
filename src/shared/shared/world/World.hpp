#pragma once

#include <cassert>
#include <glm/glm.hpp>

namespace onion::voxel
{
	enum class Biome : uint8_t
	{
		Ocean,
		Plains,
		Forest,
		Desert,
		Mountain,
		Snow,

		Count,
	};

} // namespace onion::voxel
