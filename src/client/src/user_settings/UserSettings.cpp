#include "UserSettings.hpp"

#include <fstream>
#include <iostream>

namespace onion::voxel
{
	UserSettings UserSettings::Load(const std::filesystem::path& filePath)
	{
		UserSettings settings; // defaults

		// If file does not exist → return defaults
		if (!std::filesystem::exists(filePath))
		{
			std::cerr << "UserSettings file not found at " << filePath << ". Using default settings.\n";
			return settings;
		}

		try
		{
			std::ifstream file(filePath);
			if (!file.is_open())
			{
				std::cerr << "Failed to open UserSettings file: " << filePath << "\n";
				return settings;
			}

			nlohmann::json j;
			file >> j;

			settings = j.get<UserSettings>();
		}
		catch (const std::exception& e)
		{
			// Optional: log error
			std::cerr << "Failed to load UserSettings: " << e.what() << std::endl;
			// Return defaults
		}

		return settings;
	}

	void UserSettings::Save(const UserSettings& settings, const std::filesystem::path& filePath)
	{
		try
		{
			// Ensure directory exists
			if (filePath.has_parent_path())
			{
				std::filesystem::create_directories(filePath.parent_path());
			}

			std::ofstream file(filePath);
			if (!file.is_open())
			{
				std::cerr << "Failed to open file for saving UserSettings\n";
				return;
			}

			nlohmann::ordered_json j = settings;

			// Pretty print (4 spaces)
			file << j.dump(4);
		}
		catch (const std::exception& e)
		{
			// Optional: log error
			std::cerr << "Failed to save UserSettings: " << e.what() << std::endl;
		}
	}
} // namespace onion::voxel
