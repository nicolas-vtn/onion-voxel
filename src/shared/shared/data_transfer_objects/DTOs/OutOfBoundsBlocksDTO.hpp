#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/vector.hpp>

#include <shared/data_transfer_objects/serializer/GlmSerialization.hpp>

#include "BlockDTO.hpp"

namespace onion::voxel
{
	struct OutOfBoundsBlocksDTO
	{
		std::unordered_map<glm::ivec2, std::vector<BlockDTO>> OutOfBoundsBlocks;

		template <class Archive> void serialize(Archive& ar) { ar(OutOfBoundsBlocks); }
	};
} // namespace onion::voxel
