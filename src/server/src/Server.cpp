#include "Server.hpp"

#include <iostream>

namespace onion::voxel
{
	Server::Server() : m_WorldManager(std::make_shared<WorldManager>()), m_WorldGenerator(m_WorldManager)
	{
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
	}

	void Server::Handle_PlayerInfoMsgReceived(const NetworkServer::MessageReceivedEventArgs& args,
											  const PlayerInfoMsg& msg)
	{
		std::cout << "Received PlayerInfoMsg from client " << args.Sender << ": Username=" << msg.Username
				  << ", UUID=" << msg.UUID << ", Position=" << msg.Position.x << "," << msg.Position.y << ","
				  << msg.Position.z << "\n";
	}

	void Server::Handle_ClientConnected(const NetworkServer::ClientConnectedEventArgs& args)
	{
		std::cout << "New client connected: " << args.Client << " (" << args.Username << ", " << args.UUID << ", "
				  << args.IpAddress << ")\n";

		// Send a welcome message to the newly connected client
		ServerInfoMsg srvInfoMsg;
		srvInfoMsg.ServerName = "Demo_Server";
		srvInfoMsg.ClientHandle = args.Client;

		m_NetworkServer.Send(args.Client, srvInfoMsg);

		// DEMO : Generate 10 * 10 chunks around (0, 0)
		for (int x = -5; x < 5; ++x)
		{
			for (int z = -5; z < 5; ++z)
			{
				m_WorldGenerator.GenerateChunkAsync({x, z});
			}
		}
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

	void Server::SubscribeToWorldManagerEvents()
	{
		m_WorldManagerEventHandles.push_back(m_WorldManager->ChunkAdded.Subscribe(
			[this](const std::shared_ptr<Chunk>& chunk) { Handle_ChunkAdded(chunk); }));

		m_WorldManagerEventHandles.push_back(m_WorldManager->ChunkRemoved.Subscribe(
			[this](const std::shared_ptr<Chunk>& chunk) { Handle_ChunkRemoved(chunk); }));
	}

} // namespace onion::voxel
