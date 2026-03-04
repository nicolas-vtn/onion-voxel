#include "Server.hpp"

#include <iostream>

namespace onion::voxel
{
	Server::Server()
	{
		SubscribeToNetworkServerEvents();
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
			[this](const NetworkServer::ClientConnectedEventArgs& args) { HandleClientConnected(args); }));

		m_NetworkServerEventHandles.push_back(m_NetworkServer.ClientDisconnected.Subscribe(
			[this](const NetworkServer::ClientDisconnectedEventArgs& args) { HandleClientDisconnected(args); }));
	}

	void Server::HandleClientConnected(const NetworkServer::ClientConnectedEventArgs& args)
	{
		std::cout << "New client connected: " << args.Client << " (" << args.Username << ", " << args.UUID << ", "
				  << args.IpAddress << ")\n";

		// Send a welcome message to the newly connected client
		ServerInfoMsg srvInfoMsg;
		srvInfoMsg.ServerName = "Demo_Server";
		srvInfoMsg.ClientHandle = args.Client;

		m_NetworkServer.Send(args.Client, srvInfoMsg);
	}

	void Server::HandleClientDisconnected(const NetworkServer::ClientDisconnectedEventArgs& args)
	{
		std::cout << "Client disconnected: " << args.Client << " ( " << args.UUID << ", " << args.IpAddress << ")\n";
	}

} // namespace onion::voxel
