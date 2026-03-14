#pragma once

#include <glm/glm.hpp>

namespace onion::voxel::WorldConstants
{
#ifdef NDEBUG
	constexpr uint8_t CHUNK_SIZE = 64;
#else
	constexpr uint8_t CHUNK_SIZE = 16;
#endif
} // namespace onion::voxel::WorldConstants
