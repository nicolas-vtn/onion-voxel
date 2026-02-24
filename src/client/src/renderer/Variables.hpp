#pragma once

#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace onion::voxel
{
	inline std::filesystem::path GetAssetsPath()
	{
		std::filesystem::path currentPath = std::filesystem::current_path();
		std::filesystem::path assetsPath = currentPath / "assets";
		if (!std::filesystem::exists(assetsPath))
		{
			std::cerr << "WARNING: Assets directory not found at: " << assetsPath << std::endl;
			throw std::runtime_error("Assets directory not found at: " + assetsPath.string());
		}
		return assetsPath;
	}

	inline std::filesystem::path GetMinecraftAssetsPath()
	{
		std::filesystem::path currentPath = std::filesystem::current_path();
		std::filesystem::path minecraftAssetsPath = currentPath / "assets" / "minecraft_32";
		if (!std::filesystem::exists(minecraftAssetsPath))
		{
			std::cerr << "WARNING: Minecraft assets directory not found at: " << minecraftAssetsPath << std::endl;
			throw std::runtime_error("Minecraft assets directory not found at: " + minecraftAssetsPath.string());
		}
		return minecraftAssetsPath;
	}
} // namespace onion::voxel
