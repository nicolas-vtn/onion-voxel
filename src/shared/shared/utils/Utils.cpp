#include "Utils.hpp"

#include <Windows.h>

namespace onion::voxel::Utils
{

	std::filesystem::path GetExecutableDirectory()
	{
		char buffer[MAX_PATH];
		GetModuleFileNameA(NULL, buffer, MAX_PATH);
		std::filesystem::path executablePath(buffer);
		return executablePath.parent_path();
	}

} // namespace onion::voxel::Utils
