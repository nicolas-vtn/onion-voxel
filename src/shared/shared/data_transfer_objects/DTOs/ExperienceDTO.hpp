#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>

namespace onion::voxel
{
	struct ExperienceDTO
	{
		uint32_t Value = 0;

		template <class Archive> void serialize(Archive& ar) { ar(Value); }
	};
} // namespace onion::voxel
