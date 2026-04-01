#include "Utils.hpp"

#include <Windows.h>

#include <algorithm>
#include <cctype>
#include <string>

namespace onion::voxel::Utils
{

	std::filesystem::path GetExecutableDirectory()
	{
		char buffer[MAX_PATH];
		GetModuleFileNameA(NULL, buffer, MAX_PATH);
		std::filesystem::path executablePath(buffer);
		return executablePath.parent_path();
	}

	std::string SanitizeFileName(const std::string& name)
	{
		std::string result;
		result.reserve(name.size());

		// Windows-invalid filename characters:
		// < > : " / \ | ? *  and control chars [0..31]
		for (unsigned char ch : name)
		{
			if (ch < 32 || ch == '<' || ch == '>' || ch == ':' || ch == '"' || ch == '/' || ch == '\\' || ch == '|' ||
				ch == '?' || ch == '*' || ch == ' ')
			{
				result += '_';
			}
			else
			{
				result += static_cast<char>(ch);
			}
		}

		// Trim trailing spaces and dots (invalid on Windows)
		while (!result.empty() && (result.back() == ' ' || result.back() == '.'))
		{
			result.pop_back();
		}

		// Avoid empty filename
		if (result.empty())
		{
			return "untitled";
		}

		// Check reserved Windows device names (case-insensitive)
		auto toUpper = [](std::string s)
		{
			std::transform(
				s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
			return s;
		};

		// Reserved names are checked on the basename before the first dot
		std::string baseName = result;
		size_t dotPos = baseName.find('.');
		if (dotPos != std::string::npos)
		{
			baseName = baseName.substr(0, dotPos);
		}

		std::string upperBase = toUpper(baseName);

		if (upperBase == "CON" || upperBase == "PRN" || upperBase == "AUX" || upperBase == "NUL" ||
			upperBase == "COM1" || upperBase == "COM2" || upperBase == "COM3" || upperBase == "COM4" ||
			upperBase == "COM5" || upperBase == "COM6" || upperBase == "COM7" || upperBase == "COM8" ||
			upperBase == "COM9" || upperBase == "LPT1" || upperBase == "LPT2" || upperBase == "LPT3" ||
			upperBase == "LPT4" || upperBase == "LPT5" || upperBase == "LPT6" || upperBase == "LPT7" ||
			upperBase == "LPT8" || upperBase == "LPT9")
		{
			result = "_" + result;
		}

		return result;
	}

} // namespace onion::voxel::Utils
