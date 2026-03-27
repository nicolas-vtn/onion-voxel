#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>

#include <shared/network_messages/GlmSerialization.hpp>

namespace onion::voxel
{
	struct TransformDTO
	{
		glm::vec3 Position{};
		glm::vec3 Rotation{};
		glm::vec3 Scale{1.0f, 1.0f, 1.0f};

		template <class Archive> void serialize(Archive& ar) { ar(Position, Rotation, Scale); }
	};
} // namespace onion::voxel
