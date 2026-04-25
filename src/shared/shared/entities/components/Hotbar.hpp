#pragma once

#include <array>

#include <shared/world/block/BlockId.hpp>

namespace onion::voxel
{
	struct Hotbar
	{
		std::array<BlockId, 9> Slots{BlockId::Air};
		uint8_t SelectedSlot = 0;
	};
} // namespace onion::voxel
