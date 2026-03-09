#include "Server.hpp"

#include <iostream>

#include <shared/network_messages/Serializer.hpp>
#include <shared/utils/Utils.hpp>

namespace onion::voxel
{
	Server::Server() : m_WorldManager(std::make_shared<WorldManager>()), m_WorldGenerator(m_WorldManager)
	{
		LoadConfiguration();

		SubscribeToNetworkServerEvents();
		SubscribeToWorldManagerEvents();
	}

	Server::~Server()
	{
		Stop();
	}

	void Server::Start()
	{
		m_NetworkServer.Start();
		m_IsRunning.store(true);
	}

	void Server::Stop()
	{
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
				if constexpr (std::is_same_v<T, PlayerInfoMsg>)
				{
					Handle_PlayerInfoMsgReceived(args, msg);
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
		std::cout << "Received ClientInfoMsg from client " << args.Sender << ": Username=" << msg.Username
				  << ", UUID=" << msg.UUID << "\n";

		// Update player info
		PlayerInfo playerInfo;
		playerInfo.ClientHandle = args.Sender;
		playerInfo.Username = msg.Username;
		playerInfo.UUID = msg.UUID;

		AddOrUpdatePlayer(playerInfo);
	}

	void Server::Handle_PlayerInfoMsgReceived(const NetworkServer::MessageReceivedEventArgs& args,
											  const PlayerInfoMsg& msg)
	{
		std::cout << "Received PlayerInfoMsg from client " << args.Sender << ": Username=" << msg.Username
				  << ", UUID=" << msg.UUID << ", Position=" << msg.Position.x << "," << msg.Position.y << ","
				  << msg.Position.z << "\n";

		// Update player position
		UpdatePlayerPosition(args.Sender, msg.Position);
	}

	void Server::Handle_ClientConnected(const NetworkServer::ClientConnectedEventArgs& args)
	{
		std::cout << "New client connected: " << args.Client << " (" << args.Username << ", " << args.UUID << ", "
				  << args.IpAddress << ")\n";

		// Send a welcome message to the newly connected client
		ServerInfoMsg srvInfoMsg;
		srvInfoMsg.ServerName = m_Config.serverData.ServerName;
		srvInfoMsg.ClientHandle = args.Client;
		srvInfoMsg.SimulationDistance = m_Config.serverData.SimulationDistance;

		m_NetworkServer.Send(args.Client, srvInfoMsg);

		//// DEMO : Generate 10 * 10 chunks around (0, 0)
		//for (int x = -5; x < 5; ++x)
		//{
		//	for (int z = -5; z < 5; ++z)
		//	{
		//		m_WorldGenerator.GenerateChunkAsync({x, z});
		//	}
		//}
	}

	void Server::Handle_ClientDisconnected(const NetworkServer::ClientDisconnectedEventArgs& args)
	{
		std::cout << "Client disconnected: " << args.Client << " ( " << args.UUID << ", " << args.IpAddress << ")\n";
	}

	void Server::Handle_ChunkAdded(const std::shared_ptr<Chunk>& chunk)
	{
		// Sends the new chunk to all connected clients
		ChunkDataMsg chunkDataMsg = Serializer::SerializeChunk(chunk);
		m_NetworkServer.Broadcast(chunkDataMsg);
	}

	void Server::Handle_ChunkRemoved(const std::shared_ptr<Chunk>& chunk)
	{
		// For now, the client has the resposiblity to remove it's chunks.
	}

	void Server::Handle_BlocksChanged(const std::vector<std::pair<glm::ivec3, Block>>& changedBlocks)
	{
		BlocksChangedMsg blocksChangedMsg;
		for (const auto& [worldPos, block] : changedBlocks)
		{
			blocksChangedMsg.ChangedBlocks.emplace_back(worldPos, Serializer::SerializeBlock(block));
		}

		m_NetworkServer.Broadcast(blocksChangedMsg);
	}

	void Server::GenerateChunksAroundPlayer(const glm::ivec2& playerChunkPosition)
	{
		const int simulationDistance = m_Config.serverData.SimulationDistance;
		for (int x = playerChunkPosition.x - simulationDistance; x <= playerChunkPosition.x + simulationDistance; ++x)
		{
			for (int z = playerChunkPosition.y - simulationDistance; z <= playerChunkPosition.y + simulationDistance;
				 ++z)
			{
				m_WorldGenerator.GenerateChunkAsync({x, z});
			}
		}
	}

	void Server::RemoveUnusedChunks()
	{
		// One unused chunk is a chunk that is outside the simulation distance of all players.
		std::vector<glm::ivec2> chunksToRemove;
		{
			std::shared_lock lock(m_MutexPlayers);
			for (const auto& [chunkPos, chunk] : m_WorldManager->GetAllChunks())
			{
				bool isChunkUsed = false;
				for (const auto& [clientHandle, playerChunkPos] : m_PlayerChunkPositions)
				{
					const int simulationDistance = m_Config.serverData.SimulationDistance;
					if (std::abs(playerChunkPos.x - chunkPos.x) <= simulationDistance &&
						std::abs(playerChunkPos.y - chunkPos.y) <= simulationDistance)
					{
						isChunkUsed = true;
						break;
					}
				}
				if (!isChunkUsed)
				{
					chunksToRemove.push_back(chunkPos);
				}
			}
		}

		for (const auto& chunkPos : chunksToRemove)
		{
			m_WorldManager->RemoveChunk(chunkPos);
		}
	}

	void Server::AddOrUpdatePlayer(const PlayerInfo& playerInfo)
	{
		std::lock_guard lock(m_MutexPlayers);
		m_Players[playerInfo.ClientHandle] = playerInfo;
		m_PlayerChunkPositions[playerInfo.ClientHandle] = Utils::WorldToChunkPosition(playerInfo.Position);
	}

	void Server::UpdatePlayerPosition(uint32_t clientHandle, const glm::vec3& newPosition)
	{
		glm::ivec2 playerChunkPos;
		glm::ivec2 oldChunkPos;

		{
			std::lock_guard lock(m_MutexPlayers);

			auto it = m_Players.find(clientHandle);
			if (it == m_Players.end())
			{
				throw std::runtime_error("Player with client handle " + std::to_string(clientHandle) + " not found.");
			}

			oldChunkPos = m_PlayerChunkPositions[clientHandle];
			playerChunkPos = Utils::WorldToChunkPosition(newPosition);

			it->second.Position = newPosition;
			m_PlayerChunkPositions[clientHandle] = playerChunkPos;
		}

		// lock released here

		if (playerChunkPos != oldChunkPos)
		{
			RemoveUnusedChunks();
			GenerateChunksAroundPlayer(playerChunkPos);
		}
	}

	void Server::RemovePlayer(uint32_t clientHandle)
	{
		std::lock_guard lock(m_MutexPlayers);
		m_Players.erase(clientHandle);
	}

	Server::PlayerInfo Server::GetPlayerInfo(uint32_t clientHandle) const
	{
		std::shared_lock lock(m_MutexPlayers);
		auto it = m_Players.find(clientHandle);
		if (it != m_Players.end())
		{
			return it->second;
		}
		else
		{
			throw std::runtime_error("Player with client handle " + std::to_string(clientHandle) + " not found.");
		}
	}

	void Server::SubscribeToWorldManagerEvents()
	{
		m_WorldManagerEventHandles.push_back(m_WorldManager->ChunkAdded.Subscribe(
			[this](const std::shared_ptr<Chunk>& chunk) { Handle_ChunkAdded(chunk); }));

		m_WorldManagerEventHandles.push_back(m_WorldManager->ChunkRemoved.Subscribe(
			[this](const std::shared_ptr<Chunk>& chunk) { Handle_ChunkRemoved(chunk); }));

		m_WorldManagerEventHandles.push_back(m_WorldManager->BlocksChanged.Subscribe(
			[this](const std::vector<std::pair<glm::ivec3, Block>>& changedBlocks)
			{ Handle_BlocksChanged(changedBlocks); }));
	}

} // namespace onion::voxel
