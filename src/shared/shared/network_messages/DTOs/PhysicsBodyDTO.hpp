#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>

#include <shared/network_messages/GlmSerialization.hpp>

namespace onion::voxel
{
	struct PhysicsBodyDTO
	{
		glm::vec3 Velocity{};
		bool OnGround = false;
		bool IsFlying = true;
		glm::vec3 HalfSize{1.f};
		glm::vec3 Offset{0.f};

		template <class Archive> void serialize(Archive& ar) { ar(Velocity, OnGround, IsFlying, HalfSize, Offset); }
	};
} // namespace onion::voxel
