#pragma once

#include <array>

#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>

#include <shared/entities/components/Inventory.hpp>

namespace onion::voxel
{
	struct InventoryDTO
	{
		int Rows = 0;
		int Columns = 0;
		std::vector<Slot> Slots;
		int SelectedIndex = -1;

		template <class Archive> void serialize(Archive& ar) { ar(Rows, Columns, Slots, SelectedIndex); }
	};
} // namespace onion::voxel

namespace cereal
{
	template <class Archive> void serialize(Archive& ar, onion::voxel::Slot& slot)
	{
		ar(slot.Id, slot.Count);
	}
} // namespace cereal
