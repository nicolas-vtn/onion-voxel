#pragma once

#include <array>

#include <shared/world/block/BlockId.hpp>

namespace onion::voxel
{
	struct Inventory
	{
		std::array<BlockId, 27> Slots{BlockId::Air};
	};
} // namespace onion::voxel
