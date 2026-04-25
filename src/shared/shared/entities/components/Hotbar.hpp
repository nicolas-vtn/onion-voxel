#pragma once

#include <array>

#include <shared/world/block/BlockId.hpp>

namespace onion::voxel
{
	struct Hotbar
	{
		static constexpr size_t MaxSlots = 9;
		std::array<BlockId, MaxSlots> Slots{BlockId::Air};
		uint8_t SelectedSlot = 0;

		size_t GetSelectedBlockIndex() const { return static_cast<size_t>(SelectedSlot); }
	};
} // namespace onion::voxel
