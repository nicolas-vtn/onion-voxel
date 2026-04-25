#pragma once

#include <array>

#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/array.hpp>

#include <shared/world/block/BlockId.hpp>

namespace onion::voxel
{
	struct HotbarDTO
	{
		std::array<BlockId, 9> Slots{BlockId::Air};
		uint8_t SelectedSlot = 0;

		template <class Archive> void serialize(Archive& ar) { ar(Slots, SelectedSlot); }
	};
} // namespace onion::voxel
