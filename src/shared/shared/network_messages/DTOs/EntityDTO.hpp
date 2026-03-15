#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>

#include <shared/network_messages/GlmSerialization.hpp>

namespace onion::voxel
{
	struct EntityDTO
	{
		int Type = 0;
		glm::vec3 Position{};
		glm::vec3 Facing{};

		std::string Name;
		std::string UUID;

		template <class Archive> void serialize(Archive& ar) { ar(Type, Position, Facing, Name, UUID); }
	};
} // namespace onion::voxel
