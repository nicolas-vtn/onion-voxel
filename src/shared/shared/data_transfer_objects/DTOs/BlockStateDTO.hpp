#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>

namespace onion::voxel
{
	struct BlockStateDTO
	{
		uint16_t id;
		uint8_t variantIndex;

		template <class Archive> void serialize(Archive& ar) { ar(id, variantIndex); }
	};
} // namespace onion::voxel
