#pragma once

#include <filesystem>
#include <string>

#include <onion/DateTime.hpp>

#include <shared/world/world_generator/WorldGenerator.hpp>

namespace onion::voxel
{
	struct WorldInfos
	{
		std::string Version;
		std::string Name;
		uint32_t Seed{0};
		DateTime CreationDate;
		DateTime LastPlayedDate;
		WorldGenerator::eWorldGenerationType WorldGenerationType = WorldGenerator::eWorldGenerationType::Superflat;

		std::filesystem::path SaveDirectory;
	};
} // namespace onion::voxel
