#pragma once

#include <optional>

#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/optional.hpp>

#include <shared/data_transfer_objects/serializer/GlmSerialization.hpp>

#include "ExperienceDTO.hpp"
#include "HealthDTO.hpp"
#include "HotbarDTO.hpp"
#include "HungerDTO.hpp"
#include "InventoryDTO.hpp"
#include "PhysicsBodyDTO.hpp"
#include "TransformDTO.hpp"

namespace onion::voxel
{
	struct EntityDTO
	{
		int Type = 0;
		std::string UUID;
		uint8_t State = 0;

		std::optional<TransformDTO>   Transform;
		std::optional<PhysicsBodyDTO> PhysicsBody;
		std::optional<HealthDTO>      Health;
		std::optional<HungerDTO>      Hunger;
		std::optional<ExperienceDTO>  Experience;
		std::optional<HotbarDTO>      Hotbar;
		std::optional<InventoryDTO>   Inventory;

		template <class Archive>
		void serialize(Archive& ar)
		{
			ar(Type, UUID, State, Transform, PhysicsBody, Health, Hunger, Experience, Hotbar, Inventory);
		}
	};
} // namespace onion::voxel
