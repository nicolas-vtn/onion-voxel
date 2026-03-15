#pragma once

#include <shared/entities/entity/Entity.hpp>

namespace onion::voxel
{
	class Player : public Entity
	{
	  public:
		Player(const std::string& uuid, const std::string& username) : Entity(EntityType::Player, uuid, username) {}

		~Player() override {}
	};
} // namespace onion::voxel
