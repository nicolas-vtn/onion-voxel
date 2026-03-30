#include "WorldSave.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include <shared/utils/Utils.hpp>

#include <shared/data_transfer_objects/serializer/SerializerDTO.hpp>

namespace onion::voxel
{
	WorldSave::WorldSave(const std::filesystem::path& saveDirectory) : m_SaveDirectory(saveDirectory)
	{
		m_Infos = LoadInfos(saveDirectory);

		// Start periodic task to save chunks every m_SavePeriodSeconds seconds
		m_TimerSave.setTimeoutFunction([this]() { SavePeriodically(); });
		m_TimerSave.setElapsedPeriod(std::chrono::seconds(m_SavePeriodSeconds));
		m_TimerSave.Start();
	}

	WorldSave::~WorldSave() {}

	uint32_t WorldSave::GetSeed() const
	{
		return m_Infos.Seed;
	}

	WorldGenerator::eWorldGenerationType WorldSave::GetWorldGenerationType() const
	{
		return m_Infos.WorldGenerationType;
	}

	void WorldSave::CreateWorld(const std::filesystem::path& saveDirectory, const WorldInfos& infos)
	{

		if (std::filesystem::exists(saveDirectory))
		{
			std::cerr << "Save directory already exists: " << saveDirectory << "\n";
			throw std::runtime_error("Save directory already exists: " + saveDirectory.string());
		}

		std::filesystem::create_directories(saveDirectory);
		std::filesystem::create_directories(saveDirectory / s_ChunksDirectoryName);

		SaveInfos(saveDirectory, infos);
	}

	bool WorldSave::GetWorldInfos(const std::filesystem::path& saveDirectory, WorldInfos& outInfos)
	{
		try
		{
			outInfos = LoadInfos(saveDirectory);
			return true;
		}
		catch (const std::exception& e)
		{
			std::cerr << "Failed to load world infos: " << e.what() << "\n";
			return false;
		}
	}

	void WorldSave::SaveChunkAsync(const std::shared_ptr<Chunk>& chunk)
	{
		std::lock_guard lock(m_MutexChunksToSave);
		m_ChunksToSave[chunk->GetPosition()] = chunk;
	}

	std::shared_ptr<Chunk> WorldSave::LoadChunk(const glm::ivec2& chunkPosition)
	{
		// First check if the chunk is in the chunks to save map.
		{
			std::lock_guard lock(m_MutexChunksToSave);
			auto it = m_ChunksToSave.find(chunkPosition);
			if (it != m_ChunksToSave.end())
			{
				auto chunk = it->second;
				m_ChunksToSave.erase(it);
				return chunk;
			}
		}

		{
			std::vector<uint8_t> chunkData;
			{
				std::lock_guard lock(m_MutexDiskAccess);
				std::string chunkFileName = GetChunkFileName(chunkPosition);
				std::filesystem::path chunkFilePath = GetChunkFilePath(m_SaveDirectory, chunkPosition);

				if (std::filesystem::exists(chunkFilePath))
				{
					std::ifstream file(chunkFilePath, std::ios::binary);
					if (!file.is_open())
					{
						std::cerr << "Failed to open chunk file for reading: " << chunkFilePath << "\n";
						throw std::runtime_error("Failed to open chunk file for reading: " + chunkFilePath.string());
					}
					chunkData = std::vector<uint8_t>(std::istreambuf_iterator<char>(file), {});
				}
				else
				{
					return nullptr;
				}
			}

			if (chunkData.empty())
				return nullptr;

			// Create a stream from the raw buffer
			std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
			ss.write(reinterpret_cast<const char*>(chunkData.data()), chunkData.size());
			ss.seekg(0); // reset read position

			// Deserialize with cereal
			cereal::BinaryInputArchive archive(ss);

			ChunkDTO dto;
			archive(dto);

			std::shared_ptr<Chunk> chunk = SerializerDTO::DeserializeChunk(dto);

			return chunk;
		}
	}

	void WorldSave::SavePlayersAsync(const std::unordered_map<std::string, std::shared_ptr<Player>>& players)
	{
		std::lock_guard lock(m_MutexPlayersToSave);
		for (const auto& [playerName, player] : players)
		{
			m_PlayersToSave[playerName] = player;
		}
	}

	std::shared_ptr<Player> WorldSave::LoadPlayer(const std::string& playerName)
	{
		// First check if the player is in the players to save map.
		{
			std::lock_guard lock(m_MutexPlayersToSave);
			auto it = m_PlayersToSave.find(playerName);
			if (it != m_PlayersToSave.end())
			{
				auto player = it->second;
				m_PlayersToSave.erase(it);
				return player;
			}
		}

		return nullptr; // Player loading not implemented yet
	}

	void WorldSave::SavePeriodically()
	{
		SaveChunks();
		SavePlayers();
	}

	void WorldSave::SaveChunks()
	{
		std::unordered_map<glm::ivec2, std::shared_ptr<Chunk>> chunksToSaveCopy;
		{
			std::lock_guard lock(m_MutexChunksToSave);
			chunksToSaveCopy = m_ChunksToSave;
			m_ChunksToSave.clear();
		}

		std::vector<std::pair<std::filesystem::path, std::vector<uint8_t>>> chunksDataToWrite;
		for (const auto& [chunkPos, chunk] : chunksToSaveCopy)
		{
			const std::string chunkFileName = GetChunkFileName(chunkPos);
			std::filesystem::path chunkFilePath = GetChunkFilePath(m_SaveDirectory, chunkPos);

			ChunkDTO chunkDto = SerializerDTO::SerializeChunk(chunk);
			std::ostringstream stream(std::ios::binary);
			cereal::BinaryOutputArchive archive(stream);
			archive(chunkDto);
			std::string chunkDataStr = stream.str();
			std::vector<uint8_t> chunkData(chunkDataStr.begin(), chunkDataStr.end());

			chunksDataToWrite.emplace_back(chunkFilePath, std::move(chunkData));
		}

		{
			std::lock_guard lock(m_MutexDiskAccess);
			for (const auto& [chunkFilePath, chunkData] : chunksDataToWrite)
			{
				std::filesystem::create_directories(chunkFilePath.parent_path());
				std::ofstream file(chunkFilePath, std::ios::binary);
				if (!file.is_open())
				{
					std::cerr << "Failed to open chunk file for writing: " << chunkFilePath << "\n";
					throw std::runtime_error("Failed to open chunk file for writing: " + chunkFilePath.string());
				}
				file.write(reinterpret_cast<const char*>(chunkData.data()), chunkData.size());
			}
		}
	}

	void WorldSave::SavePlayers() {}

	void WorldSave::SaveInfos(const std::filesystem::path& saveDirectory, const WorldInfos& infos)
	{
		std::filesystem::path infosFilePath = saveDirectory / s_InfosFileName;

		nlohmann::ordered_json json;
		json["Name"] = infos.Name;
		json["Seed"] = infos.Seed;
		json["CreationDate"] = infos.CreationDate.toUnixTimestamp();
		json["WorldGenerationType"] = static_cast<uint8_t>(infos.WorldGenerationType);

		std::ofstream file(infosFilePath);
		if (!file.is_open())
		{
			std::cerr << "Failed to open infos file for writing: " << infosFilePath << "\n";
			throw std::runtime_error("Failed to open infos file for writing: " + infosFilePath.string());
		}

		file << json.dump(4);
	}

	WorldInfos WorldSave::LoadInfos(const std::filesystem::path& saveDirectory)
	{

		std::filesystem::path infosFilePath = saveDirectory / s_InfosFileName;

		if (!std::filesystem::exists(infosFilePath))
		{
			std::cerr << "Infos file does not exist: " << infosFilePath << "\n";
			throw std::runtime_error("Infos file does not exist: " + infosFilePath.string());
		}

		std::ifstream file(infosFilePath);
		if (!file.is_open())
		{
			std::cerr << "Failed to open infos file for reading: " << infosFilePath << "\n";
			throw std::runtime_error("Failed to open infos file for reading: " + infosFilePath.string());
		}

		nlohmann::ordered_json json;
		file >> json;

		WorldInfos infos;
		infos.Name = json["Name"].get<std::string>();
		infos.Seed = json["Seed"].get<uint32_t>();
		infos.CreationDate = DateTime::FromUnixTimestamp(json["CreationDate"].get<uint32_t>());
		infos.WorldGenerationType =
			static_cast<WorldGenerator::eWorldGenerationType>(json["WorldGenerationType"].get<uint8_t>());

		return infos;
	}

	std::string WorldSave::GetChunkFileName(const glm::ivec2& chunkPosition)
	{
		return std::to_string(chunkPosition.x) + "_" + std::to_string(chunkPosition.y);
	}

	std::string WorldSave::GetRegionDirectoryName(const glm::ivec2& chunkPosition)
	{
		glm::ivec2 regionPos{Utils::FloorDiv(chunkPosition.x, s_RegionSizeInChunks),
							 Utils::FloorDiv(chunkPosition.y, s_RegionSizeInChunks)};

		return s_RegionDirectoryNamePrefix + std::to_string(regionPos.x) + "_" + std::to_string(regionPos.y);
	}

	std::filesystem::path WorldSave::GetChunkFilePath(const std::filesystem::path& saveDirectory,
													  const glm::ivec2& chunkPosition)
	{
		std::string regionDirName = GetRegionDirectoryName(chunkPosition);
		std::filesystem::path regionDirPath = saveDirectory / s_ChunksDirectoryName / regionDirName;
		std::string chunkFileName = GetChunkFileName(chunkPosition);
		return regionDirPath / chunkFileName;
	}

} // namespace onion::voxel
