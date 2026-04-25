#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>

namespace onion::voxel
{
	struct HealthDTO
	{
		int CurrentHealth = 20;

		template <class Archive> void serialize(Archive& ar) { ar(CurrentHealth); }
	};
} // namespace onion::voxel
