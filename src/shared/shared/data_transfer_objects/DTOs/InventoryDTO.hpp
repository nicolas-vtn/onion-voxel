#pragma once

#include <array>

#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/array.hpp>

#include <shared/world/block/BlockId.hpp>

namespace onion::voxel
{
	struct InventoryDTO
	{
		std::array<BlockId, 27> Slots{BlockId::Air};

		template <class Archive> void serialize(Archive& ar) { ar(Slots); }
	};
} // namespace onion::voxel
