#include "NetworkServer.hpp"

#include <iostream>
#include <stdexcept>

#include <shared/init_enet_once/InitEnetOnce.hpp>

namespace onion::voxel
{
	NetworkServer::NetworkServer(int port) : m_Port(port)
	{
		InitEnetOnce::Init();
	}

	NetworkServer::~NetworkServer()
	{
		Stop();
	}

	void NetworkServer::Start()
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		ENetAddress address;
		address.host = ENET_HOST_ANY; // Bind to all interfaces
		address.port = m_Port;

		m_EnetServer = enet_host_create(&address, // Address
										32,		  // Max clients
										2,		  // Channels
										0,		  // Incoming bandwidth (0 = unlimited)
										0		  // Outgoing bandwidth
		);

		if (!m_EnetServer)
		{
			std::cerr << "Failed to create ENet server.\n";
			throw std::runtime_error("Failed to create ENet server.");
		}

		std::cout << "Server started on port " << m_Port << "...\n";

		ENetEvent event;

		// Start the event listening thread
		m_EventThread = std::jthread([this](std::stop_token stopToken) { ListenForEvents(stopToken); });

		m_IsRunning.store(true);
	}

	void NetworkServer::Stop()
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		// Signal the event thread to stop and wait for it to finish
		if (m_EventThread.joinable())
		{
			m_EventThread.request_stop();
			m_EventThread.join();
		}

		// Destroy the ENet server
		if (m_EnetServer)
		{
			enet_host_destroy(m_EnetServer);
			m_EnetServer = nullptr;
		}

		m_IsRunning.store(false);
	}

	bool NetworkServer::IsRunning() const noexcept
	{
		return m_IsRunning.load();
	}
	void NetworkServer::ListenForEvents(std::stop_token stopToken)
	{
		ENetEvent event;

		ENetPacket* packet = nullptr;

		while (!stopToken.stop_requested())
		{
			while (enet_host_service(m_EnetServer, &event, 1000) > 0)
			{
				switch (event.type)
				{
					case ENET_EVENT_TYPE_CONNECT:
						std::cout << "Client connected: " << event.peer->address.host << ":" << event.peer->address.port
								  << "\n";
						break;

					case ENET_EVENT_TYPE_RECEIVE:
						std::cout << "Received: " << event.packet->data << "\n";

						// Echo back message
						packet =
							enet_packet_create(event.packet->data, event.packet->dataLength, ENET_PACKET_FLAG_RELIABLE);

						enet_peer_send(event.peer, 0, packet);
						enet_host_flush(m_EnetServer);

						enet_packet_destroy(event.packet);
						break;

					case ENET_EVENT_TYPE_DISCONNECT:
						std::cout << "Client disconnected.\n";
						event.peer->data = nullptr;
						break;

					default:
						break;
				}
			}
		}
	}
} // namespace onion::voxel
