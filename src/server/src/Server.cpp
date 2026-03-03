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
			[this](const NetworkServer::ClientHandle& client) { HandleClientConnected(client); }));

		m_NetworkServerEventHandles.push_back(m_NetworkServer.ClientDisconnected.Subscribe(
			[this](const NetworkServer::ClientHandle& client) { HandleClientDisconnected(client); }));
	}

	void Server::HandleClientConnected(NetworkServer::ClientHandle client)
	{
		std::cout << "Client connected: " << client << std::endl;

		// Send a welcome message to the newly connected client
		ServerInfoMsg welcomeMsg;
		welcomeMsg.Msg = "Welcome to the server! Your client handle is " + std::to_string(client) + ".";

		// Send in an other thread to avoid blocking the event handling thread
		std::thread([this, client, welcomeMsg]() { m_NetworkServer.Send(client, welcomeMsg); }).detach();
	}

	void Server::HandleClientDisconnected(NetworkServer::ClientHandle client)
	{
		std::cout << "Client disconnected: " << client << std::endl;
	}

} // namespace onion::voxel
