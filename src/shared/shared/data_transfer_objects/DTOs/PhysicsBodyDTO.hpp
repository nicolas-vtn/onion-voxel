#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>

#include <shared/data_transfer_objects/serializer/GlmSerialization.hpp>

namespace onion::voxel
{
	struct PhysicsBodyDTO
	{
		glm::vec3 Velocity{};
		bool OnGround = false;
		bool IsFlying = true;
		glm::vec3 Size{1.f};
		glm::vec3 CenterOffset{0.f};

		template <class Archive> void serialize(Archive& ar) { ar(Velocity, OnGround, IsFlying, Size, CenterOffset); }
	};
} // namespace onion::voxel
