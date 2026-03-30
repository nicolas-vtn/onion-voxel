#include "Server.hpp"

#include <iostream>

#include <shared/network_messages/SerializerDTO.hpp>
#include <shared/utils/Utils.hpp>

#include <windows.h>
namespace
{
	inline std::filesystem::path GetExecutableDirectory()
	{
		wchar_t buffer[MAX_PATH];
		DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);

		if (length == 0 || length == MAX_PATH)
		{
			throw std::runtime_error("Failed to get executable path");
		}

		std::filesystem::path exePath(buffer);
		return exePath.parent_path();
	}
} // namespace

namespace onion::voxel
{
	Server::Server()
	{
		LoadConfiguration();

		std::filesystem::path worldDirectory = GetExecutableDirectory() / "world";

		if (!std::filesystem::exists(worldDirectory))
		{
			WorldInfos infos;
			infos.Seed = m_Config.serverData.Seed;
			infos.Name = m_Config.serverData.ServerName;
			infos.CreationDate = DateTime::UtcNow();
			infos.WorldGenerationType =
				static_cast<WorldGenerator::eWorldGenerationType>(m_Config.serverData.WorldGenerationType);
			WorldSave::CreateWorld(worldDirectory, infos);
		}

		m_WorldManager = std::make_shared<WorldManager>(worldDirectory, false);

		// Apply Configuration
		m_WorldManager->SetChunkPersistanceDistance(m_Config.serverData.SimulationDistance);
		m_WorldManager->SetServerSimulationDistance(m_Config.serverData.SimulationDistance);

		SubscribeToNetworkServerEvents();
		SubscribeToWorldManagerEvents();

		// Setup Timer
		m_TimerSendEvents.setTimeoutFunction([this]() { Handle_TimerSendEvents(); });
		std::chrono::milliseconds defaultElapsedPeriod(100);
		m_TimerSendEvents.setElapsedPeriod(defaultElapsedPeriod);
	}

	Server::~Server()
	{
		m_WorldManager->ClearWorld();

		m_NetworkServerEventHandles.clear();
		m_WorldManagerEventHandles.clear();

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

	void Server::SubscribeToNetworkServerEvents()
	{
		m_NetworkServerEventHandles.push_back(m_NetworkServer.ClientConnected.Subscribe(
			[this](const NetworkServer::ClientConnectedEventArgs& args) { Handle_ClientConnected(args); }));

		m_NetworkServerEventHandles.push_back(m_NetworkServer.ClientDisconnected.Subscribe(
			[this](const NetworkServer::ClientDisconnectedEventArgs& args) { Handle_ClientDisconnected(args); }));

		m_NetworkServerEventHandles.push_back(m_NetworkServer.MessageReceived.Subscribe(
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
		//std::cout << "Received RequestChunksMsg from client " << args.Sender << ": Requested "
		//		  << msg.requestedChunks.size() << " chunks\n";

		for (const auto& chunkPos : msg.requestedChunks)
		{
			auto chunk = m_WorldManager->GetChunk(chunkPos);
			if (chunk)
			{
				ChunkDataMsg chunkDataMsg = SerializerDTO::SerializeChunk(chunk);
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

		// Arbitrary Set player position to 8 , 20, 8
		m_WorldManager->SetPlayerPosition(args.UUID, glm::vec3(8.0f, 20.0f, 8.0f));

		ServerInfoMsg srvInfoMsg;
		srvInfoMsg.ServerName = m_Config.serverData.ServerName;
		srvInfoMsg.ClientHandle = args.Client;
		srvInfoMsg.SimulationDistance = m_Config.serverData.SimulationDistance;

		// Sends the ServerInfoMsg to the newly connected client
		m_NetworkServer.Send(args.Client, srvInfoMsg);

		// Sends the Entity snapshot to all clients
		Handle_TimerSendEvents();
	}

	void Server::Handle_ClientDisconnected(const NetworkServer::ClientDisconnectedEventArgs& args)
	{
		std::cout << "Client disconnected: " << args.Client << " ( " << args.UUID << ", " << args.IpAddress << ")\n";

		RemovePlayer(args.UUID);
	}

	void Server::Handle_ChunkAdded(const std::shared_ptr<Chunk>& chunk)
	{
		// Sends the new chunk to all connected clients
		ChunkDataMsg chunkDataMsg = SerializerDTO::SerializeChunk(chunk);
		m_NetworkServer.Broadcast(chunkDataMsg);
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

		std::shared_ptr<Player> player = std::make_shared<Player>(playerInfo.UUID);
		player->SetName(playerInfo.PlayerName);

		m_WorldManager->AddPlayer(player);

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
		m_WorldManagerEventHandles.push_back(m_WorldManager->ChunkAdded.Subscribe(
			[this](const std::shared_ptr<Chunk>& chunk) { Handle_ChunkAdded(chunk); }));

		m_WorldManagerEventHandles.push_back(m_WorldManager->ChunkRemoved.Subscribe(
			[this](const std::shared_ptr<Chunk>& chunk) { Handle_ChunkRemoved(chunk); }));

		m_WorldManagerEventHandles.push_back(m_WorldManager->BlocksChanged.Subscribe(
			[this](const WorldManager::BlocksChangedEventArgs& args) { Handle_BlocksChanged(args); }));
	}

} // namespace onion::voxel
