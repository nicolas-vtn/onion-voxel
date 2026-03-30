#pragma once

#include <string>

#include <onion/DateTime.hpp>

#include <shared/world/world_generator/WorldGenerator.hpp>

namespace onion::voxel
{
	struct WorldInfos
	{
		uint32_t Seed{0};
		std::string Name;
		DateTime CreationDate;
		WorldGenerator::eWorldGenerationType WorldGenerationType = WorldGenerator::eWorldGenerationType::Superflat;
	};
} // namespace onion::voxel
