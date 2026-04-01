#include "Client.hpp"

#include <iostream>

#include <shared/data_transfer_objects/Serializer/SerializerDTO.hpp>
#include <shared/utils/Utils.hpp>

namespace onion::voxel
{
	Client::Client() : m_Logger(m_LogFile.string()), m_Renderer(m_WorldManager)
	{
		LoadConfiguration();

		SetLogLevel(m_LogLevel);

		SubscribeToRendererEvents();
		SubscribeToNetworkClientEvents();
		SubscribeToWorldManagerEvents();

		m_TimerSendPlayerInfos.setTimeoutFunction([this]() { SendPlayerInfosToServer(); });
		m_TimerSendPlayerInfos.setElapsedPeriod(std::chrono::milliseconds(50));
		m_TimerSendPlayerInfos.setRepeat(true);
	}

	Client::~Client()
	{
		m_RendererEventHandles.clear();
		m_NetworkClientEventHandles.clear();
		m_WorldManagerEventHandles.clear();

		Stop();
	}

	void Client::Start()
	{
		std::cout << "Start Client" << std::endl;
		m_Renderer.Start();
	}

	void Client::Stop()
	{
		std::cout << "Stop Client" << std::endl;
		m_Renderer.Stop();
	}

	void Client::Wait() {}

	bool Client::IsRunning() const noexcept
	{
		bool isRendererRunning = m_Renderer.IsRunning();
		return isRendererRunning;
	}

	void Client::SetLogLevel(eLogLevel logLevel)
	{
		m_LogLevel = logLevel;

		switch (logLevel)
		{
			case eLogLevel::All:
				m_Logger.SetLogInfos(true);
				m_Logger.SetLogErrors(true);
				break;
			case eLogLevel::ErrorsOnly:
				m_Logger.SetLogInfos(false);
				m_Logger.SetLogErrors(true);
				break;
			case eLogLevel::None:
				m_Logger.SetLogInfos(false);
				m_Logger.SetLogErrors(false);
				break;
		}
	}

	Client::eLogLevel Client::GetLogLevel() const
	{
		return m_LogLevel;
	}

	void Client::LoadConfiguration()
	{
		m_Config.Load(m_ConfigFilePath);

		// Apply configuration to the client
		m_WorldManager->SetChunkPersistanceDistance(m_Config.clientData.RenderDistance);
		m_Renderer.SetPlayerUUID(m_Config.clientData.UUID);
	}

	void Client::SaveConfiguration()
	{
		m_Config.Save(m_ConfigFilePath);
	}

	void Client::Handle_StartSingleplayerRequest(const WorldInfos& worldInfos)
	{
		m_NetworkClient.SetRemoteHost("127.0.0.1");
		m_NetworkClient.SetRemotePort(7777);

		// Starts a Server on Localhost
		if (m_LocalhostServer == nullptr)
		{
			m_LocalhostServer = std::make_unique<Server>(worldInfos.SaveDirectory);
			m_LocalhostServer->Start();
		}
		else
		{
			throw std::runtime_error("Localhost Server is already running");
		}

		// Connects to Localhost Server
		if (!m_NetworkClient.IsRunning())
		{
			m_NetworkClient.Start();
			m_TimerSendPlayerInfos.Start();
		}
		else
		{
			throw std::runtime_error("Network Client is already running");
		}

		// Sets Renderer UI to InGame UI.
		m_Renderer.SetRenderState(Renderer::eRenderState::InGame);

		// Sends a message to Server
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		ClientInfoMsg clientInfoMsg;
		clientInfoMsg.PlayerName = m_Config.clientData.PlayerName;
		clientInfoMsg.UUID = m_Config.clientData.UUID;

		m_NetworkClient.Send(std::move(clientInfoMsg), true);
	}

	void Client::Handle_StartMultiplayerRequest(const Gui::MultiplayerGameStartInfo& multiplayerGameStartInfo)
	{
		m_NetworkClient.SetRemoteHost(multiplayerGameStartInfo.ServerAddress);
		m_NetworkClient.SetRemotePort(multiplayerGameStartInfo.ServerPort);

		if (!m_NetworkClient.IsRunning())
		{
			m_NetworkClient.Start();
			m_TimerSendPlayerInfos.Start();
		}
		else
		{
			throw std::runtime_error("Network Client is already running");
		}

		// Sets Renderer UI to InGame UI.
		m_Renderer.SetRenderState(Renderer::eRenderState::InGame);

		// Sends a message to Server
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		ClientInfoMsg clientInfoMsg;
		clientInfoMsg.PlayerName = m_Config.clientData.PlayerName;
		clientInfoMsg.UUID = m_Config.clientData.UUID;

		m_NetworkClient.Send(std::move(clientInfoMsg), true);
	}

	void Client::Handle_StopPlayingRequest(const std::filesystem::path& worldPath)
	{
		(void) worldPath; // Currently unused

		// Stops Network Client
		if (m_NetworkClient.IsRunning())
		{
			m_TimerSendPlayerInfos.Stop();
			m_NetworkClient.Stop();
		}

		// Stops Localhost Server if it was started (Singleplayer)
		if (m_LocalhostServer != nullptr)
		{
			m_LocalhostServer->Stop();
			m_LocalhostServer.reset();
		}

		m_WorldManager->ClearWorld();

		m_Renderer.SetRenderState(Renderer::eRenderState::Menu);
	}

	void Client::SubscribeToWorldManagerEvents()
	{
		m_WorldManagerEventHandles.push_back(m_WorldManager->MissingChunksRequested.Subscribe(
			[this](const std::vector<glm::ivec2>& chunkPositions) { Handle_MissingChunksRequested(chunkPositions); }));

		m_WorldManagerEventHandles.push_back(m_WorldManager->BlocksChanged.Subscribe(
			[this](const WorldManager::BlocksChangedEventArgs& args) { Handle_BlocksChanged(args); }));
	}

	void Client::Handle_MissingChunksRequested(const std::vector<glm::ivec2>& chunkPositions)
	{
		RequestChunksMsg requestChunksMsg;
		requestChunksMsg.requestedChunks = chunkPositions;

		//std::cout << "Requesting " << chunkPositions.size() << " missing chunks from server\n";

		m_NetworkClient.Send(std::move(requestChunksMsg), false);
	}

	void Client::Handle_BlocksChanged(const WorldManager::BlocksChangedEventArgs& args)
	{
		// Do not send back to server ()
		bool isOutOfBoundsPlaced = (args.Origin == WorldManager::BlocksChangedEventArgs::eOrigin::OutOfBoundsPlaced);
		bool isServerRequest = (args.Origin == WorldManager::BlocksChangedEventArgs::eOrigin::ServerRequest);
		if (isOutOfBoundsPlaced || isServerRequest)
		{
			return;
		}

		std::vector<BlockDTO> changedBlocksDTO;
		for (const auto& block : args.ChangedBlocks)
		{
			changedBlocksDTO.emplace_back(SerializerDTO::SerializeBlock(block));
		}

		BlocksChangedMsg blocksChangedMsg;
		blocksChangedMsg.ChangedBlocks = std::move(changedBlocksDTO);

		m_NetworkClient.Send(std::move(blocksChangedMsg), true);
	}

	void Client::SubscribeToRendererEvents()
	{
		m_RendererEventHandles.push_back(m_Renderer.RequestStartSingleplayerGame.Subscribe(
			[this](const WorldInfos& worldInfos) { Handle_StartSingleplayerRequest(worldInfos); }));

		m_RendererEventHandles.push_back(m_Renderer.RequestQuitToMainMenu.Subscribe(
			[this](bool quit)
			{
				(void) quit;
				Handle_StopPlayingRequest("");
			}));

		m_RendererEventHandles.push_back(m_Renderer.RequestStartMultiplayerGame.Subscribe(
			[this](const Gui::MultiplayerGameStartInfo& multiplayerGameStartInfo)
			{ Handle_StartMultiplayerRequest(multiplayerGameStartInfo); }));
	}

	void Client::SubscribeToNetworkClientEvents()
	{
		m_NetworkClientEventHandles.push_back(m_NetworkClient.Connected.Subscribe(
			[this](const ServerInfoMsg& message) { Handle_ClientConnected(message); }));

		m_NetworkClientEventHandles.push_back(m_NetworkClient.Disconnected.Subscribe(
			[this](bool disconnectedByServer) { Handle_ClientDisconnected(disconnectedByServer); }));

		m_NetworkClientEventHandles.push_back(m_NetworkClient.MessageReceived.Subscribe(
			[this](const NetworkMessage& message) { Handle_NetworkMessageReceived(message); }));
	}

	void Client::Handle_ClientConnected(const ServerInfoMsg& message)
	{
		Handle_ServerInfoMessageReceived(message);
	}

	void Client::Handle_ClientDisconnected(bool disconnectedByServer)
	{
		(void) disconnectedByServer; // Currently unused
		m_Renderer.SetServerInfo(nullptr);
	}

	void Client::Handle_NetworkMessageReceived(const NetworkMessage& message)
	{
		std::visit(
			[this](const auto& msg)
			{
				using T = std::decay_t<decltype(msg)>;
				if constexpr (std::is_same_v<T, ServerInfoMsg>)
				{
					Handle_ServerInfoMessageReceived(msg);
				}
				else if constexpr (std::is_same_v<T, ChunkDataMsg>)
				{
					Handle_ChunkDataMessageReceived(msg);
				}
				else if constexpr (std::is_same_v<T, BlocksChangedMsg>)
				{
					Handle_BlocksChangedMessageReceived(msg);
				}
				else if constexpr (std::is_same_v<T, EntitySnapshotMsg>)
				{
					Handle_EntitySnapshotMessageReceived(msg);
				}
				else
				{
					std::cout << "Received unhandled message type from server\n";
				}
			},
			message);
	}

	void Client::Handle_ServerInfoMessageReceived(const ServerInfoMsg& msg)
	{
		std::cout << "Received ServerInfoMsg: ServerName=" << msg.ServerName << ", ClientHandle=" << msg.ClientHandle
				  << std::endl;

		m_ClientHandle = msg.ClientHandle;
		m_ServerName = msg.ServerName;

		std::shared_ptr<ServerInfo> serverInfo = std::make_shared<ServerInfo>();
		serverInfo->Name = msg.ServerName;
		serverInfo->Address = m_NetworkClient.GetRemoteHost();
		serverInfo->Port = m_NetworkClient.GetRemotePort();
		serverInfo->SimulationDistance = msg.SimulationDistance;

		m_Renderer.SetServerInfo(serverInfo);
		m_WorldManager->SetServerSimulationDistance(msg.SimulationDistance);
	}

	void Client::Handle_ChunkDataMessageReceived(const ChunkDataMsg& msg)
	{
		std::shared_ptr<Chunk> chunk = SerializerDTO::DeserializeChunk(msg.Chunk);

		// Checks if the chunk is in the persistance distance
		glm::ivec2 chunkPosition = chunk->GetPosition();
		glm::ivec2 playerChunkPosition = Utils::WorldToChunkPosition(m_Renderer.GetPlayerPosition());

		//std::cout << "Received chunk at position: (" << chunkPosition.x << ", " << chunkPosition.y << ")\n";

		const int persistanceDistance = m_WorldManager->GetChunkPersistanceDistance();

		if (std::abs(playerChunkPosition.x - chunkPosition.x) <= persistanceDistance &&
			std::abs(playerChunkPosition.y - chunkPosition.y) <= persistanceDistance)
		{
			m_WorldManager->AddChunk(chunk);
		}
	}

	void Client::Handle_BlocksChangedMessageReceived(const BlocksChangedMsg& msg)
	{
		std::vector<Block> changedBlocks;
		for (const auto& blockDTO : msg.ChangedBlocks)
		{
			changedBlocks.emplace_back(SerializerDTO::DeserializeBlock(blockDTO));
		}

		//std::cout << "Received BlocksChangedMsg: " << changedBlocks.size() << " changed blocks\n";

		m_WorldManager->SetBlocks(changedBlocks, WorldManager::BlocksChangedEventArgs::eOrigin::ServerRequest, true);
	}

	void Client::Handle_EntitySnapshotMessageReceived(const EntitySnapshotMsg& msg)
	{
		//std::cout << "Received EntitySnapshotMsg: " << msg.Entities.size() << " entities\n";

		std::vector<std::shared_ptr<Entity>> players;
		for (const auto& playerDTO : msg.Players)
		{
			// Deserialize the player and add it to the list of players to update in the EntityManager
			std::shared_ptr<Player> player = SerializerDTO::DeserializePlayer(playerDTO);

			// If the entity is the player itself, update only if player not present in entities manager
			if (player->UUID == m_Config.clientData.UUID)
			{
				const bool hasPlayerBeenSet = m_WorldManager->GetPlayer(m_Config.clientData.UUID) != nullptr;
				if (!hasPlayerBeenSet)
				{
					// If the player has not been set yet, add it to the EntityManager
					m_WorldManager->AddPlayer(player);
				}

				continue;
			}

			players.push_back(player);
		}

		std::vector<std::shared_ptr<Entity>> entities;
		for (const auto& entityDTO : msg.Entities)
		{
			// Deserialize the entity and add it to the list of entities to update in the EntityManager
			std::shared_ptr<Entity> entity = SerializerDTO::DeserializeEntity(entityDTO);
			entities.push_back(entity);
		}

		// Remove the players that are not present in the received snapshot
		std::unordered_map<std::string, std::shared_ptr<Player>> currentPlayers = m_WorldManager->GetAllPlayers();
		for (const auto& [uuid, player] : currentPlayers)
		{
			if (uuid == m_Config.clientData.UUID)
			{
				continue; // Skip the player itself
			}
			auto it = std::find_if(players.begin(),
								   players.end(),
								   [&uuid](const std::shared_ptr<Entity>& entity) { return entity->UUID == uuid; });
			if (it == players.end())
			{
				std::cout << "Removing player with UUID " << uuid << " from EntityManager\n";
				m_WorldManager->RemovePlayer(uuid);
			}
		}

		m_WorldManager->UpdateEntities(players);
		m_WorldManager->UpdateEntities(entities);
	}

	void Client::SendPlayerInfosToServer()
	{
		// If the player position is not initialized yet, do not send infos to server
		std::shared_ptr<Player> player = m_Renderer.GetPlayer();
		if (!player)
		{
			return;
		}

		PlayerInfoMsg playerInfoMsg;
		playerInfoMsg.player = SerializerDTO::SerializePlayer(*player);

		m_NetworkClient.Send(std::move(playerInfoMsg), false);
	}

} // namespace onion::voxel
