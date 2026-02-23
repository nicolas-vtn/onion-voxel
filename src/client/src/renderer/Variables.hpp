#pragma once

#include <filesystem>
#include <iostream>

namespace onion::voxel
{
	inline std::filesystem::path GetAssetsPath()
	{
		std::filesystem::path currentPath = std::filesystem::current_path();
		std::filesystem::path assetsPath = currentPath / "assets";
		if (!std::filesystem::exists(assetsPath))
		{
			std::cerr << "WARNING: Assets directory not found at: " << assetsPath << std::endl;
			return "";
		}
		return assetsPath;
	}
} // namespace onion::voxel
