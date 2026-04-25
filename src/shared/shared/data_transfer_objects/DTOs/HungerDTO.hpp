#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>

namespace onion::voxel
{
	struct HungerDTO
	{
		int CurrentHunger = 20;

		template <class Archive> void serialize(Archive& ar) { ar(CurrentHunger); }
	};
} // namespace onion::voxel
