#include "NetworkServer.hpp"

#include <iostream>
#include <stdexcept>

#include <shared/init_enet_once/InitEnetOnce.hpp>

namespace onion::voxel
{
	NetworkServer::NetworkServer(uint16_t port) : m_Port(port)
	{
		InitEnetOnce::Init();

		m_DispatchIncomingMessagesThread =
			std::jthread([this](std::stop_token stopToken) { DispatchIncomingMessages(stopToken); });
	}

	NetworkServer::~NetworkServer()
	{
		Stop();

		if (m_DispatchIncomingMessagesThread.joinable())
		{
			m_DispatchIncomingMessagesThread.request_stop();
			m_DispatchIncomingMessagesThread.join();
		}

		std::cout << "NetworkServer destroyed.\n";
	}

	void NetworkServer::Start()
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		ENetAddress address;
		address.host = ENET_HOST_ANY; // Bind to all interfaces
		address.port = static_cast<enet_uint16>(m_Port);

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

		std::cout << "Network server stopped.\n";
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
		if (!m_IsRunning)
			return;

		OutgoingMessage out;
		out.Targets = clients;
		out.Message = std::move(message);
		out.Reliable = reliable;

		m_OutgoingMessages.Push(std::move(out));
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

	uint16_t NetworkServer::GetServerPort() const
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		return m_Port;
	}

	void NetworkServer::SetServerPort(uint16_t port)
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_Port = port;

		if (IsRunning())
		{
			Stop();
			Start();
		}
	}

	void NetworkServer::ListenForEvents(std::stop_token stopToken)
	{
		ENetEvent event;
		ClientSession session;

		while (!stopToken.stop_requested())
		{
			ProcessOutgoingMessages();

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
										// Extracts ClientInfoMessage
										ClientInfoMsg clientInfo = std::get<ClientInfoMsg>(msg);

										// Get client's IP address
										char ip[64];
										enet_address_get_host_ip(&event.peer->address, ip, sizeof(ip));

										// Update session with authenticated client info
										it->second.authenticated = true;
										it->second.uuid = clientInfo.UUID;
										it->second.ipAddress = ip;

										// Trigger ClientConnected event
										ClientConnectedEventArgs args;
										args.Client = handle;
										args.UUID = clientInfo.UUID;
										args.IpAddress = ip;
										args.Username = clientInfo.Username;

										ClientConnected.Trigger(args);
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
							std::unique_lock<std::mutex> lock(m_ClientMutex);
							ClientSession& retrevedSession = m_PeerToSession[event.peer];

							// Build event args before erasing session data
							ClientDisconnectedEventArgs disconnectArgs;
							disconnectArgs.Client = retrevedSession.handle;
							disconnectArgs.UUID = retrevedSession.uuid;
							disconnectArgs.IpAddress = retrevedSession.ipAddress;

							// Clean up session data
							m_HandleToPeer.erase(retrevedSession.handle);
							m_PeerToSession.erase(event.peer);

							lock.unlock();

							// Trigger ClientDisconnected event
							ClientDisconnected.Trigger(disconnectArgs);
						}

						break;

					default:
						break;
				}
			}
		}
	}

	std::vector<uint8_t> NetworkServer::SerializeNetworkMessage(const NetworkMessage& message)
	{
		std::ostringstream stream(std::ios::binary);
		cereal::BinaryOutputArchive archive(stream);

		std::visit(
			[&](auto&& msg)
			{
				using T = std::decay_t<decltype(msg)>;

				MessageHeader header;
				header.Type = T::StaticType;
				header.ClientHandle = 0;

				archive(header);
				archive(msg);
			},
			message);

		std::string tmp = stream.str();

		return std::vector<uint8_t>(tmp.begin(), tmp.end());
	}

	void NetworkServer::ProcessOutgoingMessages()
	{
		OutgoingMessage msg;

		while (m_OutgoingMessages.TryPop(msg))
		{
			std::vector<uint8_t> buffer = SerializeNetworkMessage(msg.Message);

			const enet_uint32 flags = msg.Reliable ? ENET_PACKET_FLAG_RELIABLE : 0;

			std::lock_guard<std::mutex> lock(m_ClientMutex);

			for (ClientHandle handle : msg.Targets)
			{
				auto it = m_HandleToPeer.find(handle);
				if (it == m_HandleToPeer.end())
					continue;

				ENetPacket* packet = enet_packet_create(buffer.data(), buffer.size(), flags);

				enet_peer_send(it->second, 0, packet);
			}
		}

		enet_host_flush(m_EnetServer);
	}

	NetworkServer::ClientHandle NetworkServer::GenerateClientHandle()
	{
		std::lock_guard<std::mutex> lock(m_ClientMutex);
		return m_NextClientHandle++;
	}

	void NetworkServer::DispatchIncomingMessages(std::stop_token stopToken)
	{

		IncommingMessage msg;
		MessageReceivedEventArgs args;

		while (m_IncomingMessages.WaitPop(msg, stopToken))
		{
			args.Sender = msg.Sender;
			args.Message = std::move(msg.Message);
			MessageReceived.Trigger(args);
		}
	}
} // namespace onion::voxel
