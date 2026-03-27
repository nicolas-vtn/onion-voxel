#pragma once

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>

namespace onion::voxel
{
	struct ClientData
	{
		std::string PlayerName = "Default";
		std::string UUID;
		uint8_t RenderDistance = 8;
	};

	struct ClientConfiguration
	{
		// ----- Members -----
	  public:
		ClientData clientData;

		// ----- Constructor / Destructor -----
	  public:
		ClientConfiguration() = default;
		~ClientConfiguration() = default;

		// ----- Load / Save -----
	  public:
		void Load(const std::filesystem::path& filePath)
		{
			if (!std::filesystem::exists(filePath))
			{
				// Generate a random UUID
				std::random_device rd;
				std::mt19937 gen(rd());
				std::uniform_int_distribution<> dis(0, 15);
				std::string uuid;
				for (int i = 0; i < 32; i++)
					uuid += "0123456789abcdef"[dis(gen)];
				clientData.UUID = uuid;

				// Save the new configuration
				Save(filePath);

				return;
			}

			std::ifstream file(filePath);
			if (!file.is_open())
			{
				std::cerr << "Failed to open configuration file: " << filePath << "\n";
				throw std::runtime_error("Failed to open configuration file: " + filePath.string());
			}

			nlohmann::json json;

			file >> json;

			clientData.PlayerName = json.value("PlayerName", "");
			clientData.UUID = json.value("UUID", "");
			clientData.RenderDistance = json.value("RenderDistance", static_cast<uint8_t>(8));

			// ------- DEBUG ONLY : Regenerate UUID -------
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<> dis(0, 15);
			std::string uuid;
			for (int i = 0; i < 32; i++)
				uuid += "0123456789abcdef"[dis(gen)];
			clientData.UUID = uuid;
		}

		void Save(const std::filesystem::path& filePath) const
		{
			nlohmann::json json;

			json["PlayerName"] = clientData.PlayerName;
			json["UUID"] = clientData.UUID;
			json["RenderDistance"] = clientData.RenderDistance;

			std::ofstream file(filePath);
			if (!file.is_open())
			{
				std::cerr << "Failed to open configuration file for writing: " << filePath << "\n";
				throw std::runtime_error("Failed to open configuration file for writing: " + filePath.string());
			}

			file << json.dump(4); // Pretty print with 4 spaces indentation
		}
	};
} // namespace onion::voxel
