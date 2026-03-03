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
		// Signal the event thread to stop and wait for it to finish
		if (m_EventThread.joinable())
		{
			m_EventThread.request_stop();
			m_EventThread.join();
		}

		std::lock_guard<std::mutex> lock(m_Mutex);

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

	void NetworkServer::Send(ClientHandle client, NetworkMessage message, bool reliable)
	{
		Send(std::vector<ClientHandle>{client}, std::move(message), reliable);
	}

	void NetworkServer::Send(const std::vector<ClientHandle>& clients, NetworkMessage message, bool reliable)
	{
		if (!m_EnetServer)
			return;

		// ---- Serialize once ----
		std::ostringstream stream(std::ios::binary);
		cereal::BinaryOutputArchive archive(stream);

		std::visit(
			[&](auto&& msg)
			{
				using T = std::decay_t<decltype(msg)>;

				MessageHeader header;
				header.Type = T::StaticType;
				header.SenderId = "Server";

				archive(header);
				archive(msg);
			},
			message);

		std::string buffer = stream.str();

		const enet_uint32 flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;

		// ---- Send to each client ----
		std::lock_guard<std::mutex> lock(m_ClientMutex);

		for (ClientHandle handle : clients)
		{
			auto it = m_HandleToPeer.find(handle);
			if (it == m_HandleToPeer.end())
				continue;

			ENetPeer* peer = it->second;

			ENetPacket* packet = enet_packet_create(buffer.data(), buffer.size(), flags);

			enet_peer_send(peer, 0, packet);
		}

		enet_host_flush(m_EnetServer);
	}

	void NetworkServer::Broadcast(NetworkMessage message, bool reliable)
	{
		std::vector<ClientHandle> clients;

		{
			std::lock_guard<std::mutex> lock(m_ClientMutex);

			clients.reserve(m_HandleToPeer.size());
			for (const auto& [handle, _] : m_HandleToPeer)
				clients.push_back(handle);
		}

		Send(clients, std::move(message), reliable);
	}

	bool NetworkServer::TryPopMessage(ServerEvent& out)
	{
		return m_IncomingMessages.TryPop(out);
	}

	void NetworkServer::ListenForEvents(std::stop_token stopToken)
	{
		ENetEvent event;
		ClientSession session;

		while (!stopToken.stop_requested())
		{
			while (enet_host_service(m_EnetServer, &event, 1) > 0)
			{
				switch (event.type)
				{
					case ENET_EVENT_TYPE_CONNECT:
						std::cout << "Client connected: " << event.peer->address.host << ":" << event.peer->address.port
								  << "\n";

						session = ClientSession();
						session.handle = GenerateClientHandle();
						session.peer = event.peer;

						{
							std::lock_guard<std::mutex> lock(m_ClientMutex);
							m_PeerToSession[event.peer] = session;
							m_HandleToPeer[session.handle] = event.peer;
						}

						break;

					case ENET_EVENT_TYPE_RECEIVE:
						{
							const auto* rawData = reinterpret_cast<const char*>(event.packet->data);
							const auto dataSize = static_cast<std::size_t>(event.packet->dataLength);

							struct MemoryStream : std::streambuf
							{
								MemoryStream(const char* data, std::size_t size)
								{
									char* ptr = const_cast<char*>(data);
									setg(ptr, ptr, ptr + size);
								}
							};

							MemoryStream memStream(rawData, dataSize);
							std::istream stream(&memStream);

							cereal::BinaryInputArchive archive(stream);

							try
							{
								MessageHeader header;
								archive(header);

								NetworkMessage msg = DeserializeMessage(archive, header.Type);

								ClientHandle handle;

								{
									std::lock_guard<std::mutex> lock(m_ClientMutex);
									auto it = m_PeerToSession.find(event.peer);
									if (it == m_PeerToSession.end())
										break;

									handle = it->second.handle;

									if (!it->second.authenticated && header.Type != MessageHeader::eType::ClientInfo)
										break;

									if (header.Type == MessageHeader::eType::ClientInfo)
									{
										it->second.authenticated = true;
										it->second.uuid = header.SenderId;

										ClientConnected.Trigger(handle);
									}
								}

								m_IncomingMessages.Push({handle, std::move(msg)});
							}
							catch (const std::exception& e)
							{
								std::cerr << "Invalid packet: " << e.what() << "\n";
							}

							enet_packet_destroy(event.packet);
							break;
						}

					case ENET_EVENT_TYPE_DISCONNECT:
						std::cout << "Client disconnected.\n";
						event.peer->data = nullptr;

						{
							std::lock_guard<std::mutex> lock(m_ClientMutex);
							ClientSession& session = m_PeerToSession[event.peer];
							m_HandleToPeer.erase(session.handle);
							m_PeerToSession.erase(event.peer);
						}

						break;

					default:
						break;
				}
			}
		}
	}

	NetworkServer::ClientHandle NetworkServer::GenerateClientHandle()
	{

		std::lock_guard<std::mutex> lock(m_ClientMutex);
		return m_NextClientHandle++;
	}
} // namespace onion::voxel
