#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>

#include "../GlmSerialization.hpp"
#include "BlockStateDTO.hpp"

namespace onion::voxel
{
	struct BlockDTO
	{
		glm::ivec3 position{};
		BlockStateDTO state{};

		template <class Archive> void serialize(Archive& ar) { ar(position, state); }
	};
} // namespace onion::voxel
