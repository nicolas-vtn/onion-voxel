#pragma once

#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace onion::voxel
{
	inline const std::string CLIENT_VERSION = "0.1.0";

	//inline std::string SelectedResourcePackName = "Default";

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

	inline std::filesystem::path GetTexturesPath()
	{
		std::filesystem::path currentPath = std::filesystem::current_path();
		std::filesystem::path texturesPath = currentPath / "assets" / "textures";
		if (!std::filesystem::exists(texturesPath))
		{
			std::cerr << "WARNING: Textures directory not found at: " << texturesPath << std::endl;
			throw std::runtime_error("Textures directory not found at: " + texturesPath.string());
		}
		return texturesPath;
	}

	inline std::filesystem::path GetMinecraftTexturesPath()
	{
		std::filesystem::path currentPath = std::filesystem::current_path();
		std::filesystem::path minecraftTexturesPath = currentPath / "assets" / "minecraft_16" / "textures";
		if (!std::filesystem::exists(minecraftTexturesPath))
		{
			std::cerr << "WARNING: Minecraft textures directory not found at: " << minecraftTexturesPath << std::endl;
			throw std::runtime_error("Minecraft textures directory not found at: " + minecraftTexturesPath.string());
		}
		return minecraftTexturesPath;
	}

	inline std::filesystem::path GetMinecraftDataPath()
	{
		std::filesystem::path currentPath = std::filesystem::current_path();
		std::filesystem::path minecraftDataPath = currentPath / "assets" / "minecraft";
		if (!std::filesystem::exists(minecraftDataPath))
		{
			std::cerr << "WARNING: Minecraft data directory not found at: " << minecraftDataPath << std::endl;
			throw std::runtime_error("Minecraft data directory not found at: " + minecraftDataPath.string());
		}
		return minecraftDataPath;
	}

	inline std::filesystem::path GetResourcePacksDirectory()
	{
		std::filesystem::path currentPath = std::filesystem::current_path();
		std::filesystem::path resourcePacksPath = currentPath / "resourcepacks";

		// Create the resource packs directory if it doesn't exist, since users will need to place their resource packs here.
		std::filesystem::create_directories(resourcePacksPath);

		if (!std::filesystem::exists(resourcePacksPath))
		{
			std::cerr << "WARNING: Resource packs directory not found at: " << resourcePacksPath << std::endl;
			throw std::runtime_error("Resource packs directory not found at: " + resourcePacksPath.string());
		}
		return resourcePacksPath;
	}

} // namespace onion::voxel
