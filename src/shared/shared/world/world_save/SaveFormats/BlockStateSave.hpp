#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>

namespace onion::voxel
{
	struct BlockStateSave
	{
		uint16_t id;
		uint8_t facing;
		uint8_t top;

		template <class Archive> void serialize(Archive& ar) { ar(id, facing, top); }
	};
} // namespace onion::voxel
