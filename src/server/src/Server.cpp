#include "Server.hpp"

#include <iostream>

#include <shared/data_transfer_objects/serializer/SerializerDTO.hpp>
#include <shared/utils/Utils.hpp>

namespace onion::voxel
{
	Server::Server()
	{
		LoadConfiguration();

		// Create world if it doesn't exist
		m_Config.serverData.WorldDirectory = Utils::GetExecutableDirectory() / "world";
		const std::filesystem::path& worldDir = m_Config.serverData.WorldDirectory;
		if (!std::filesystem::exists(worldDir))
		{
			WorldInfos infos;
			infos.Seed = m_Config.serverData.Seed;
			infos.Name = m_Config.serverData.ServerName;
			infos.CreationDate = DateTime::UtcNow();
			infos.WorldGenerationType =
				static_cast<WorldGenerator::eWorldGenerationType>(m_Config.serverData.WorldGenerationType);
			WorldSave::CreateWorld(worldDir, infos);
		}

		Initialize();

		// ---- Set MOTD Data ----
		UpdateMOTD();
	}

	Server::Server(const ServerConfiguration& config) : m_Config(config)
	{
		Initialize();
	}

	Server::~Server()
	{
		m_NetworkServerEventHandles.clear();
		m_WorldManagerEventHandles.clear();

		m_NetworkServer.Stop();

		m_WorldManager->ClearWorld();

		Stop();
	}

	void Server::Start()
	{
		m_TimerSendEvents.Start();
		m_NetworkServer.Start();
		m_IsRunning.store(true);
	}

	void Server::Stop()
	{
		m_TimerSendEvents.Stop();
		m_NetworkServer.Stop();
		m_IsRunning.store(false);
	}

	bool Server::IsRunning() const noexcept
	{
		return m_IsRunning.load();
	}

	void Server::SetChunkLoadingDistance(uint8_t distance)
	{
		m_Config.serverData.SimulationDistance = distance;

		m_WorldManager->SetChunkLoadingDistance(distance);
		m_WorldManager->SetChunkPersistanceDistance(distance);

		// Broadcast new simulation distance to all clients
		ServerInfoMsg srvInfoMsg;
		srvInfoMsg.ServerName = m_Config.serverData.ServerName;
		srvInfoMsg.SimulationDistance = m_Config.serverData.SimulationDistance;

		// Sends the ServerInfoMsg to the newly connected client
		m_NetworkServer.Broadcast(srvInfoMsg);
	}

	void Server::Initialize()
	{
		m_NetworkServer.SetServerPort(m_Config.serverData.Port);

		m_WorldManager = std::make_shared<WorldManager>(m_Config.serverData.WorldDirectory, false);

		// Apply Configuration
		m_WorldManager->SetChunkPersistanceDistance(m_Config.serverData.SimulationDistance);
		m_WorldManager->SetChunkLoadingDistance(m_Config.serverData.SimulationDistance);

		SubscribeToNetworkServerEvents();
		SubscribeToWorldManagerEvents();

		// Setup Timer
		m_TimerSendEvents.setTimeoutFunction([this]() { Handle_TimerSendEvents(); });
		std::chrono::milliseconds defaultElapsedPeriod(100);
		m_TimerSendEvents.setElapsedPeriod(defaultElapsedPeriod);
	}

	void Server::LoadConfiguration()
	{
		m_Config.Load(m_ConfigFilePath);

		// Apply configuration to the network server
		m_NetworkServer.SetServerPort(m_Config.serverData.Port);
	}

	void Server::SaveConfiguration()
	{
		m_Config.Save(m_ConfigFilePath);
	}

	void Server::UpdateMOTD()
	{
		ServerMotdMsg motdMsg;
		motdMsg.ServerMotd = m_Config.serverData.MOTD;
		motdMsg.MaxPlayers = 20;

		auto players = m_WorldManager->GetAllPlayers();
		for (auto& [clientHandle, playerInfo] : players)
		{
			motdMsg.PlayerNames.push_back(playerInfo->GetName());
		}

		motdMsg.PlayerCount = static_cast<int>(motdMsg.PlayerNames.size());

		// Load Icon png data
		std::filesystem::path iconPath = Utils::GetExecutableDirectory() / "ServerThumbnail.png";
		if (std::filesystem::exists(iconPath))
		{
			std::ifstream file(iconPath, std::ios::binary);
			if (!file.is_open())
			{
				std::cerr << "Failed to open icon file for reading: " << iconPath << "\n";
				throw std::runtime_error("Failed to open icon file for reading: " + iconPath.string());
			}
			motdMsg.ServerIconPngData = std::vector<uint8_t>(std::istreambuf_iterator<char>(file), {});
		}
		m_NetworkServer.SetMOTD(motdMsg);
	}

	void Server::SubscribeToNetworkServerEvents()
	{
		m_NetworkServerEventHandles.push_back(m_NetworkServer.EvtClientConnected.Subscribe(
			[this](const NetworkServer::ClientConnectedEventArgs& args) { Handle_ClientConnected(args); }));

		m_NetworkServerEventHandles.push_back(m_NetworkServer.EvtClientDisconnected.Subscribe(
			[this](const NetworkServer::ClientDisconnectedEventArgs& args) { Handle_ClientDisconnected(args); }));

		m_NetworkServerEventHandles.push_back(m_NetworkServer.EvtMessageReceived.Subscribe(
			[this](const NetworkServer::MessageReceivedEventArgs& args) { Handle_NetworkMessageReceived(args); }));
	}

	void Server::Handle_NetworkMessageReceived(const NetworkServer::MessageReceivedEventArgs& args)
	{
		std::visit(
			[&](const auto& msg)
			{
				using T = std::decay_t<decltype(msg)>;
				if constexpr (std::is_same_v<T, ClientInfoMsg>)
				{
					Handle_ClientInfoMsgReceived(args, msg);
				}
				else if constexpr (std::is_same_v<T, PlayerInfoMsg>)
				{
					Handle_PlayerInfoMsgReceived(args, msg);
				}
				else if constexpr (std::is_same_v<T, RequestChunksMsg>)
				{
					Handle_PlayerRequestChunksMsgReceived(args, msg);
				}
				else if constexpr (std::is_same_v<T, BlocksChangedMsg>)
				{
					Handle_BlocksChangedMsgReceived(args, msg);
				}
				else
				{
					std::cout << "Received unhandled message type from client " << args.Sender << "\n";
				}
			},
			args.Message);
	}

	void Server::Handle_ClientInfoMsgReceived(const NetworkServer::MessageReceivedEventArgs& args,
											  const ClientInfoMsg& msg)
	{
		std::cout << "Received ClientInfoMsg from client " << args.Sender << ": PlayerName=" << msg.PlayerName
				  << ", UUID=" << msg.UUID << "\n";

		// Update player info
		PlayerInfo playerInfo;
		playerInfo.ClientHandle = args.Sender;
		playerInfo.PlayerName = msg.PlayerName;
		playerInfo.UUID = msg.UUID;

		//AddOrUpdatePlayer(playerInfo);
	}

	void Server::Handle_PlayerInfoMsgReceived(const NetworkServer::MessageReceivedEventArgs& args,
											  const PlayerInfoMsg& msg)
	{
		(void) args; // Unused parameter

		//std::cout << "Received PlayerInfoMsg from client " << args.Sender << ": Username=" << msg.Username
		//		  << ", UUID=" << msg.UUID << ", Position=" << msg.Position.x << "," << msg.Position.y << ","
		//		  << msg.Position.z << "\n";

		// Update player
		std::shared_ptr<Player> deserializedPlayer = SerializerDTO::DeserializePlayer(msg.player);

		m_WorldManager->UpdatePlayer(deserializedPlayer);
	}

	void Server::Handle_TimerSendEvents()
	{
		// Sends Entity Snapshot to all clients
		std::unordered_map<std::string, std::shared_ptr<Player>> players = m_WorldManager->GetAllPlayers();

		if (players.empty())
		{
			return; // No players connected, skip sending snapshot
		}

		EntitySnapshotMsg entitySnapshotMsg;
		for (const auto& [clientHandle, player] : players)
		{
			PlayerDTO playerDTO = SerializerDTO::SerializePlayer(*player);
			entitySnapshotMsg.Players.push_back(std::move(playerDTO));
		}

		auto entities = m_WorldManager->GetAllEntities();
		for (const auto& entity : entities)
		{
			EntityDTO entityDTO = SerializerDTO::SerializeEntity(*entity);
			entitySnapshotMsg.Entities.push_back(std::move(entityDTO));
		}

		m_NetworkServer.Broadcast(entitySnapshotMsg);
	}

	void Server::Handle_PlayerRequestChunksMsgReceived(const NetworkServer::MessageReceivedEventArgs& args,
													   const RequestChunksMsg& msg)
	{
		std::cout << "Received RequestChunksMsg from client " << args.Sender << ": Requested "
				  << msg.requestedChunks.size() << " chunks\n";

		for (const auto& chunkPos : msg.requestedChunks)
		{
			auto chunk = m_WorldManager->GetChunk(chunkPos);
			if (chunk)
			{
				ChunkDTO chunkDto = SerializerDTO::SerializeChunk(chunk);
				ChunkDataMsg chunkDataMsg;
				chunkDataMsg.Chunk = std::move(chunkDto);
				//std::cout << "(" << chunkPos.x << ", " << chunkPos.y << ")" << std::endl;
				m_NetworkServer.Send(args.Sender, chunkDataMsg);
			}
		}
	}

	void Server::Handle_BlocksChangedMsgReceived(const NetworkServer::MessageReceivedEventArgs& args,
												 const BlocksChangedMsg& msg)
	{
		// Retreve ClientId
		uint32_t clientHandle = args.Sender;

		std::cout << "Received BlocksChangedMsg from client " << clientHandle << ": " << msg.ChangedBlocks.size()
				  << " changed blocks\n";

		std::vector<Block> changedBlocks;
		for (const auto& blockDTO : msg.ChangedBlocks)
		{
			changedBlocks.emplace_back(SerializerDTO::DeserializeBlock(blockDTO));
		}

		m_WorldManager->SetBlocks(changedBlocks, WorldManager::BlocksChangedEventArgs::eOrigin::ClientRequest, true);
	}

	void Server::Handle_ClientConnected(const NetworkServer::ClientConnectedEventArgs& args)
	{
		std::cout << "New client connected: " << args.Client << " (" << args.PlayerName << ", " << args.UUID << ", "
				  << args.IpAddress << ")\n";

		PlayerInfo playerInfo;
		playerInfo.ClientHandle = args.Client;
		playerInfo.PlayerName = args.PlayerName;
		playerInfo.UUID = args.UUID;

		// Add the new Player
		AddPlayer(playerInfo);
		m_WorldManager->RequestAllMissingChunks();

		ServerInfoMsg srvInfoMsg;
		srvInfoMsg.ServerName = m_Config.serverData.ServerName;
		srvInfoMsg.ClientHandle = args.Client;
		srvInfoMsg.SimulationDistance = m_Config.serverData.SimulationDistance;

		// Sends the ServerInfoMsg to the newly connected client
		m_NetworkServer.Send(args.Client, srvInfoMsg);

		// Sends the Entity snapshot to all clients
		Handle_TimerSendEvents();

		UpdateMOTD();
	}

	void Server::Handle_ClientDisconnected(const NetworkServer::ClientDisconnectedEventArgs& args)
	{
		std::cout << "Client disconnected: " << args.Client << " ( " << args.UUID << ", " << args.IpAddress << ")\n";

		RemovePlayer(args.UUID);

		UpdateMOTD();
	}

	void Server::Handle_ChunkAdded(const std::shared_ptr<Chunk>& chunk)
	{
		// Send the chunk to all nearby players.
		uint8_t chunkLoadingDistance = m_Config.serverData.SimulationDistance;
		glm::ivec2 chunkPosition = chunk->GetPosition();
		std::unordered_map<std::string, glm::vec3> playersPosition = m_WorldManager->GetPlayersPosition();

		std::vector<std::string> nearbyPlayers;
		for (const auto& [playerUUID, playerPosition] : playersPosition)
		{
			glm::ivec2 playerChunkPos = Utils::WorldToChunkPosition(playerPosition);
			if (std::abs(playerChunkPos.x - chunkPosition.x) <= chunkLoadingDistance &&
				std::abs(playerChunkPos.y - chunkPosition.y) <= chunkLoadingDistance)
			{
				nearbyPlayers.push_back(playerUUID);
			}
		}

		if (!nearbyPlayers.empty())
		{
			ChunkDTO chunkDto = SerializerDTO::SerializeChunk(chunk);
			ChunkDataMsg chunkDataMsg;
			chunkDataMsg.Chunk = std::move(chunkDto);

			//std::cout << "Sending chunk at position: (" << chunkPosition.x << ", " << chunkPosition.y << ") to "
			//		  << nearbyPlayers.size() << " nearby players\n";

			for (const auto& playerUUID : nearbyPlayers)
			{
				uint32_t clientHandle;
				{
					std::shared_lock lock(m_MutexPlayers);
					auto it = m_UUIDToPlayerInfo.find(playerUUID);
					if (it != m_UUIDToPlayerInfo.end())
					{
						clientHandle = it->second.ClientHandle;
					}
					else
					{
						continue; // Player not found, skip sending chunk
					}
				}
				m_NetworkServer.Send(clientHandle, chunkDataMsg);
			}
		}
	}

	void Server::Handle_ChunkRemoved(const std::shared_ptr<Chunk>& chunk)
	{
		// For now, the client has the resposiblity to remove it's chunks.
		(void) chunk; // Unused parameter
	}

	void Server::Handle_BlocksChanged(const WorldManager::BlocksChangedEventArgs& args)
	{
		BlocksChangedMsg blocksChangedMsg;
		for (const auto& block : args.ChangedBlocks)
		{
			blocksChangedMsg.ChangedBlocks.emplace_back(SerializerDTO::SerializeBlock(block));
		}

		m_NetworkServer.Broadcast(blocksChangedMsg);
	}

	void Server::AddPlayer(const PlayerInfo& playerInfo)
	{
		std::shared_ptr<Player> loadedPlayer = m_WorldManager->LoadPlayer(playerInfo.UUID);

		// If the player already exists in the world manager, update the name in case it has changed since last connection
		if (loadedPlayer)
		{
			loadedPlayer->SetName(playerInfo.PlayerName);
		}
		else // Create a new player and add it to the world manager
		{
			std::shared_ptr<Player> player = std::make_shared<Player>(playerInfo.UUID);
			player->SetName(playerInfo.PlayerName);
			player->SetPosition(glm::vec3(8.f, 20.f, 8.f)); // Spawn player at a default position
			m_WorldManager->AddPlayer(player);
		}

		{
			std::lock_guard lock(m_MutexPlayers);
			m_ClientHandleToPlayerInfo[playerInfo.ClientHandle] = playerInfo;
			m_UUIDToPlayerInfo[playerInfo.UUID] = playerInfo;
		}
	}

	void Server::RemovePlayer(const std::string& uuid)
	{
		m_WorldManager->RemovePlayer(uuid);
		{
			std::lock_guard lock(m_MutexPlayers);
			auto it = m_UUIDToPlayerInfo.find(uuid);
			if (it != m_UUIDToPlayerInfo.end())
			{
				uint32_t clientHandle = it->second.ClientHandle;
				m_ClientHandleToPlayerInfo.erase(clientHandle);
				m_UUIDToPlayerInfo.erase(it);
			}
		}
	}

	void Server::SubscribeToWorldManagerEvents()
	{
		m_WorldManagerEventHandles.push_back(m_WorldManager->EvtChunkAdded.Subscribe(
			[this](const std::shared_ptr<Chunk>& chunk) { Handle_ChunkAdded(chunk); }));

		m_WorldManagerEventHandles.push_back(m_WorldManager->EvtChunkRemoved.Subscribe(
			[this](const std::shared_ptr<Chunk>& chunk) { Handle_ChunkRemoved(chunk); }));

		m_WorldManagerEventHandles.push_back(m_WorldManager->EvtBlocksChanged.Subscribe(
			[this](const WorldManager::BlocksChangedEventArgs& args) { Handle_BlocksChanged(args); }));
	}

} // namespace onion::voxel
