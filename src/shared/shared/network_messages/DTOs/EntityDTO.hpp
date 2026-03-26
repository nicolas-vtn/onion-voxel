#pragma once

#include <optional>

#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/optional.hpp>

#include <shared/network_messages/GlmSerialization.hpp>

#include "PhysicsBodyDTO.hpp"
#include "TransformDTO.hpp"

namespace onion::voxel
{
	struct EntityDTO
	{
		int Type = 0;
		std::string UUID;

		std::optional<TransformDTO> Transform;
		std::optional<PhysicsBodyDTO> PhysicsBody;

		template <class Archive> void serialize(Archive& ar) { ar(Type, UUID, Transform, PhysicsBody); }
	};
} // namespace onion::voxel
