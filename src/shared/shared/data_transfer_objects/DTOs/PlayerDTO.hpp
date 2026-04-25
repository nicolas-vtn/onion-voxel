#pragma once

#include <optional>

#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/types/string.hpp>

#include <shared/data_transfer_objects/serializer/GlmSerialization.hpp>

#include "EntityDTO.hpp"

namespace onion::voxel
{
	struct PlayerDTO : EntityDTO
	{
		PlayerDTO() = default;
		PlayerDTO(const EntityDTO& entityDTO) : EntityDTO(entityDTO) {}

		std::string Name;
		bool IsSneaking = false;

		template <class Archive> void serialize(Archive& ar)
		{
			ar(cereal::base_class<EntityDTO>(this), Name, IsSneaking);
		}
	};
} // namespace onion::voxel
