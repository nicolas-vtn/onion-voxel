#pragma once

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>

namespace onion::voxel
{
	struct ServerData
	{
		std::string ServerName = "DefaultServer";
		std::string UUID;
		uint16_t Port = 7777;
		uint8_t SimulationDistance = 8;
		uint32_t Seed = 1;
		uint8_t WorldGenerationType = 1;
	};

	struct ServerConfiguration
	{
		// ----- Members -----
	  public:
		ServerData serverData;

		// ----- Constructor / Destructor -----
	  public:
		ServerConfiguration() = default;
		~ServerConfiguration() = default;

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
				serverData.UUID = uuid;

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

			nlohmann::ordered_json json;

			file >> json;

			serverData.ServerName = json.value("ServerName", serverData.ServerName);
			serverData.UUID = json.value("UUID", serverData.UUID);
			serverData.Port = json.value("Port", serverData.Port);
			serverData.Seed = json.value("Seed", serverData.Seed);
			serverData.SimulationDistance = json.value("SimulationDistance", serverData.SimulationDistance);
			serverData.WorldGenerationType = json.value("WorldGenerationType", serverData.WorldGenerationType);
		}

		void Save(const std::filesystem::path& filePath) const
		{
			nlohmann::ordered_json json;

			json["ServerName"] = serverData.ServerName;
			json["UUID"] = serverData.UUID;
			json["Port"] = serverData.Port;
			json["Seed"] = serverData.Seed;
			json["SimulationDistance"] = serverData.SimulationDistance;
			json["WorldGenerationType"] = serverData.WorldGenerationType;

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
