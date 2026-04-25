#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>

namespace onion::voxel
{
	struct HungerDTO
	{
		float CurrentHunger = 20.0f;

		template <class Archive> void serialize(Archive& ar) { ar(CurrentHunger); }
	};
} // namespace onion::voxel
