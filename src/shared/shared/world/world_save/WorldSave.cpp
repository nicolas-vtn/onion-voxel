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
		m_TimerSave.setTimeoutFunction([this]() { SaveAll(); });
		m_TimerSave.setElapsedPeriod(std::chrono::seconds(m_SavePeriodSeconds));
		m_TimerSave.Start();
	}

	WorldSave::~WorldSave()
	{
		std::cout << "~WorldSave" << std::endl;
		m_TimerSave.Stop();
		SaveAll(); // Save one last time before destruction to minimize data loss
		std::cout << "WorldSave Saved" << std::endl;
	}

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
		std::filesystem::path validatedSaveDirectory = saveDirectory;
		while (std::filesystem::exists(validatedSaveDirectory))
		{
			// Append underscore to the directory name until we find a name that doesn't exist
			std::string newDirectoryName = validatedSaveDirectory.filename().string() + "_";
			validatedSaveDirectory = validatedSaveDirectory.parent_path() / newDirectoryName;
		}

		std::filesystem::create_directories(validatedSaveDirectory);
		std::filesystem::create_directories(validatedSaveDirectory / s_ChunksDirectoryName);

		SaveInfos(validatedSaveDirectory, infos);
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

	bool WorldSave::DeleteWorld(const WorldInfos& infos)
	{
		try
		{
			std::filesystem::remove_all(infos.SaveDirectory);
			return true;
		}
		catch (const std::exception& e)
		{
			std::cerr << "Failed to delete world: " << e.what() << "\n";
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

	void WorldSave::SavePlayersAsync(const std::unordered_map<std::string, std::shared_ptr<Player>>& players)
	{
		std::lock_guard lock(m_MutexPlayersToSave);
		for (const auto& [playerName, player] : players)
		{
			m_PlayersToSave[playerName] = player;
		}
	}

	void WorldSave::SavePlayerAsync(const std::shared_ptr<Player>& player)
	{
		std::lock_guard lock(m_MutexPlayersToSave);
		m_PlayersToSave[player->UUID] = player;
	}

	std::shared_ptr<Player> WorldSave::LoadPlayer(const std::string& playerUUID)
	{
		// First check if the player is in the players to save map.
		{
			std::lock_guard lock(m_MutexPlayersToSave);
			auto it = m_PlayersToSave.find(playerUUID);
			if (it != m_PlayersToSave.end())
			{
				auto player = it->second;
				m_PlayersToSave.erase(it);
				return player;
			}
		}

		// If not, load from disk
		std::vector<uint8_t> playerData;
		{
			std::lock_guard lock(m_MutexDiskAccess);
			std::filesystem::path playerFilePath = GetFilePathForPlayer(m_SaveDirectory, playerUUID);

			if (std::filesystem::exists(playerFilePath))
			{
				std::ifstream file(playerFilePath, std::ios::binary);
				if (!file.is_open())
				{
					std::cerr << "Failed to open player file for reading: " << playerFilePath << "\n";
					throw std::runtime_error("Failed to open player file for reading: " + playerFilePath.string());
				}
				playerData = std::vector<uint8_t>(std::istreambuf_iterator<char>(file), {});
			}
			else
			{
				// No player file found, return nullptr to indicate player doesn't exist
				return nullptr;
			}
		}

		if (playerData.empty())
			return nullptr;

		// Create a stream from the raw buffer
		std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
		ss.write(reinterpret_cast<const char*>(playerData.data()), playerData.size());
		ss.seekg(0); // reset read position

		// Deserialize with cereal
		cereal::BinaryInputArchive archive(ss);

		PlayerDTO dto;
		archive(dto);

		std::shared_ptr<Player> player = SerializerDTO::DeserializePlayer(dto);

		return player;
	}

	void WorldSave::SaveOutOfBoundsBlocks(const std::unordered_map<glm::ivec2, std::vector<Block>>& outOfBoundsBlocks)
	{
		OutOfBoundsBlocksDTO dto = SerializerDTO::SerializeOutOfBoundsBlocks(outOfBoundsBlocks);
		std::ostringstream stream(std::ios::binary);
		cereal::BinaryOutputArchive archive(stream);
		archive(dto);
		std::string dataStr = stream.str();
		std::vector<uint8_t> data(dataStr.begin(), dataStr.end());

		std::filesystem::path filePath = GetFilePathForOutOfBoundsBlocks(m_SaveDirectory);
		{
			std::lock_guard lock(m_MutexDiskAccess);
			std::filesystem::create_directories(filePath.parent_path());
			std::ofstream file(filePath, std::ios::binary);
			if (!file.is_open())
			{
				std::cerr << "Failed to open out of bounds blocks file for writing: " << filePath << "\n";
				throw std::runtime_error("Failed to open out of bounds blocks file for writing: " + filePath.string());
			}
			file.write(reinterpret_cast<const char*>(data.data()), data.size());
		}
	}

	std::unordered_map<glm::ivec2, std::vector<Block>> WorldSave::LoadOutOfBoundsBlocks()
	{
		std::vector<uint8_t> data;
		std::filesystem::path filePath = GetFilePathForOutOfBoundsBlocks(m_SaveDirectory);
		{
			std::lock_guard lock(m_MutexDiskAccess);
			if (std::filesystem::exists(filePath))
			{
				std::ifstream file(filePath, std::ios::binary);
				if (!file.is_open())
				{
					std::cerr << "Failed to open out of bounds blocks file for reading: " << filePath << "\n";
					throw std::runtime_error("Failed to open out of bounds blocks file for reading: " +
											 filePath.string());
				}
				data = std::vector<uint8_t>(std::istreambuf_iterator<char>(file), {});
			}
			else
			{
				return {};
			}
		}
		if (data.empty())
			return {};
		std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
		ss.write(reinterpret_cast<const char*>(data.data()), data.size());
		ss.seekg(0); // reset read position
		cereal::BinaryInputArchive archive(ss);
		OutOfBoundsBlocksDTO dto;
		archive(dto);
		return SerializerDTO::DeserializeOutOfBoundsBlocks(dto);
	}

	void WorldSave::SaveAll()
	{
		SaveGeneralData();
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

				std::filesystem::path tempFilePath = chunkFilePath;
				tempFilePath += ".tmp";

				std::ofstream file(tempFilePath, std::ios::binary);
				if (!file.is_open())
				{
					std::cerr << "Failed to open chunk file for writing: " << tempFilePath << "\n";
					throw std::runtime_error("Failed to open chunk file for writing: " + tempFilePath.string());
				}
				file.write(reinterpret_cast<const char*>(chunkData.data()), chunkData.size());
				file.flush();
				file.close();

				// Atomically replace the old file with the new file
				Utils::ReplaceFileAtomic(chunkFilePath, tempFilePath);
			}
		}
	}

	void WorldSave::SavePlayers()
	{
		std::unordered_map<std::string, std::shared_ptr<Player>> playersToSaveCopy;
		{
			std::lock_guard lock(m_MutexPlayersToSave);
			playersToSaveCopy = m_PlayersToSave;
			m_PlayersToSave.clear();
		}

		std::vector<std::pair<std::filesystem::path, std::vector<uint8_t>>> playersDataToWrite;
		for (const auto& [playerUUID, player] : playersToSaveCopy)
		{
			std::filesystem::path playerFilePath = GetFilePathForPlayer(m_SaveDirectory, playerUUID);

			PlayerDTO playerDto = SerializerDTO::SerializePlayer(*player);
			std::ostringstream stream(std::ios::binary);
			cereal::BinaryOutputArchive archive(stream);
			archive(playerDto);
			std::string playerDataStr = stream.str();
			std::vector<uint8_t> playerData(playerDataStr.begin(), playerDataStr.end());

			playersDataToWrite.emplace_back(playerFilePath, std::move(playerData));
		}

		{
			std::lock_guard lock(m_MutexDiskAccess);
			for (const auto& [playerFilePath, playerData] : playersDataToWrite)
			{
				std::filesystem::create_directories(playerFilePath.parent_path());
				std::ofstream file(playerFilePath, std::ios::binary);
				if (!file.is_open())
				{
					std::cerr << "Failed to open player file for writing: " << playerFilePath << "\n";
					throw std::runtime_error("Failed to open player file for writing: " + playerFilePath.string());
				}
				file.write(reinterpret_cast<const char*>(playerData.data()), playerData.size());
			}
		}
	}

	void WorldSave::SaveGeneralData()
	{
		m_Infos.LastPlayedDate = DateTime::UtcNow();
		SaveInfos(m_SaveDirectory, m_Infos);
	}

	void WorldSave::SaveInfos(const std::filesystem::path& saveDirectory, const WorldInfos& infos)
	{
		std::filesystem::path infosFilePath = saveDirectory / s_InfosFileName;

		nlohmann::ordered_json json;
		json["Version"] = s_CurrentVersion;
		json["Name"] = infos.Name;
		json["Seed"] = infos.Seed;
		json["CreationDate"] = infos.CreationDate.toUnixTimestamp();
		json["LastPlayedDate"] = infos.LastPlayedDate.toUnixTimestamp();
		json["WorldGenerationType"] = WorldGenerator::WorldGenerationTypeToString(infos.WorldGenerationType);

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
		infos.Version = json["Version"].get<std::string>();
		infos.Name = json["Name"].get<std::string>();
		infos.Seed = json["Seed"].get<uint32_t>();
		infos.CreationDate = DateTime::FromUnixTimestamp(json["CreationDate"].get<uint32_t>());
		infos.LastPlayedDate = DateTime::FromUnixTimestamp(json["LastPlayedDate"].get<uint32_t>());
		std::string worldGenTypeStr = json["WorldGenerationType"].get<std::string>();
		infos.WorldGenerationType = WorldGenerator::StringToWorldGenerationType(worldGenTypeStr);

		infos.SaveDirectory = saveDirectory;

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

	std::filesystem::path WorldSave::GetFilePathForPlayer(const std::filesystem::path& saveDirectory,
														  const std::string& playerUUID)
	{
		std::filesystem::path playersDirPath = saveDirectory / s_PlayersDirectoryName;
		return playersDirPath / playerUUID;
	}

	std::filesystem::path WorldSave::GetFilePathForOutOfBoundsBlocks(const std::filesystem::path& saveDirectory)
	{
		std::filesystem::path outOfBoundsBlocksDirPath = saveDirectory / s_OutOfBoundsBlocksDirectoryName;
		return outOfBoundsBlocksDirPath / s_OutOfBoundsBlocksFileName;
	}

} // namespace onion::voxel
