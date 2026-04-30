#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>

namespace onion::voxel
{
	struct HealthDTO
	{
		float CurrentHealth = 20.0f;

		template <class Archive> void serialize(Archive& ar) { ar(CurrentHealth); }
	};
} // namespace onion::voxel
