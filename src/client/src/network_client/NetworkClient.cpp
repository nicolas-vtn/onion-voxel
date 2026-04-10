#include "NetworkClient.hpp"

#include <iostream>
#include <stdexcept>

#include <shared/init_enet_once/InitEnetOnce.hpp>

namespace onion::voxel
{
	NetworkClient::NetworkClient(const std::string& host, uint16_t port) : m_Host(host), m_Port(port)
	{
		InitEnetOnce::Init();

		m_DispatchIncomingMessagesThread =
			std::jthread([this](std::stop_token stopToken) { DispatchIncomingMessages(stopToken); });
	}

	NetworkClient::~NetworkClient()
	{
		Stop();

		if (m_DispatchIncomingMessagesThread.joinable())
		{
			m_DispatchIncomingMessagesThread.request_stop();
			m_DispatchIncomingMessagesThread.join();
		}

		std::cout << "Network client destroyed.\n";
	}

	void NetworkClient::Start()
	{
		std::lock_guard<std::mutex> lock(m_Mutex);

		m_Client = enet_host_create(nullptr, // client
									1,		 // 1 outgoing connection
									2,		 // channels
									0,
									0);

		if (!m_Client)
			throw std::runtime_error("Failed to create ENet client.");

		ENetAddress address;
		enet_address_set_host(&address, m_Host.c_str());
		address.port = m_Port;

		m_Peer = enet_host_connect(m_Client, &address, 2, 0);

		if (!m_Peer)
			throw std::runtime_error("No available peers for connection.");

		std::cout << "Connecting to " << m_Host << ":" << m_Port << "...\n";

		m_EventThread = std::jthread([this](std::stop_token stopToken) { ListenForEvents(stopToken); });

		m_IsRunning.store(true);
	}

	void NetworkClient::Stop()
	{
		if (m_EventThread.joinable())
		{
			m_EventThread.request_stop();
			m_EventThread.join();
		}

		EvtDisconnected.Trigger(true);

		std::lock_guard<std::mutex> lock(m_Mutex);

		if (m_Peer)
		{
			enet_peer_disconnect(m_Peer, 0);

			ENetEvent event;
			bool disconnected = false;

			while (enet_host_service(m_Client, &event, 100) > 0)
			{
				if (event.type == ENET_EVENT_TYPE_DISCONNECT)
				{
					disconnected = true;
					break;
				}
			}

			if (!disconnected)
			{
				// Force disconnect if server didn't respond
				enet_peer_reset(m_Peer);
			}

			m_Peer = nullptr;
		}

		if (m_Client)
		{
			enet_host_destroy(m_Client);
			m_Client = nullptr;
		}

		m_IsRunning.store(false);

		std::cout << "Network client stopped.\n";
	}

	bool NetworkClient::IsRunning() const noexcept
	{
		return m_IsRunning.load();
	}

	void NetworkClient::Send(NetworkMessage message, bool reliable)
	{
		OutgoingMessage out;
		out.Message = std::move(message);
		out.Reliable = reliable;

		m_OutgoingMessages.Push(std::move(out));
	}

	std::string NetworkClient::GetRemoteHost() const
	{
		return m_Host;
	}

	void NetworkClient::SetRemoteHost(const std::string& host)
	{
		m_Host = host;

		if (IsRunning())
		{
			Stop();
			Start();
		}
	}

	uint16_t NetworkClient::GetRemotePort() const
	{
		return m_Port;
	}

	void NetworkClient::SetRemotePort(uint16_t port)
	{
		m_Port = port;

		if (IsRunning())
		{
			Stop();
			Start();
		}
	}

	void NetworkClient::ListenForEvents(std::stop_token stopToken)
	{
		ENetEvent event;

		while (!stopToken.stop_requested())
		{

			ProcessOutgoingMessages();

			while (enet_host_service(m_Client, &event, 1) > 0)
			{
				switch (event.type)
				{
					case ENET_EVENT_TYPE_CONNECT:
						std::cout << "Connected to server.\n";
						break;

					case ENET_EVENT_TYPE_RECEIVE:
						{
							const auto* rawData = reinterpret_cast<const char*>(event.packet->data);

							const std::size_t dataSize = static_cast<std::size_t>(event.packet->dataLength);

							struct MemoryStreamBuf : std::streambuf
							{
								MemoryStreamBuf(const char* data, std::size_t size)
								{
									char* ptr = const_cast<char*>(data);
									setg(ptr, ptr, ptr + size);
								}
							};

							MemoryStreamBuf buffer(rawData, dataSize);
							std::istream stream(&buffer);

							cereal::BinaryInputArchive archive(stream);

							try
							{
								MessageHeader header;
								archive(header);

								NetworkMessage msg = DeserializeMessage(archive, header.Type);

								if (header.Type == MessageHeader::eType::ServerInfo)
								{
									// Extracts ClientHandle from the ServerInfoMessage and updates m_ClientHandle
									ServerInfoMsg serverInfo = std::get<ServerInfoMsg>(msg);

									m_ClientHandle.store(serverInfo.ClientHandle);

									std::cout << "Assigned ClientHandle: " << m_ClientHandle.load() << "\n";

									EvtConnected.Trigger(serverInfo);
								}

								m_IncomingMessages.Push(std::move(msg));
							}
							catch (const std::exception& e)
							{
								std::cerr << "Invalid packet: " << e.what() << "\n";
							}

							enet_packet_destroy(event.packet);

							break;
						}

					case ENET_EVENT_TYPE_DISCONNECT:
						std::cout << "Disconnected from server.\n";
						EvtDisconnected.Trigger(true);
						m_Peer = nullptr;
						return;

					default:
						break;
				}
			}
		}
	}

	std::vector<uint8_t> NetworkClient::SerializeNetworkMessage(const NetworkMessage& message)
	{
		std::ostringstream stream(std::ios::binary);
		cereal::BinaryOutputArchive archive(stream);

		std::visit(
			[&](auto&& msg)
			{
				using T = std::decay_t<decltype(msg)>;

				MessageHeader header;
				header.Type = T::StaticType;
				header.ClientHandle = m_ClientHandle.load();

				archive(header);
				archive(msg);
			},
			message);

		std::string tmp = stream.str();

		return std::vector<uint8_t>(tmp.begin(), tmp.end());
	}

	void NetworkClient::ProcessOutgoingMessages()
	{
		OutgoingMessage msg;

		while (m_OutgoingMessages.TryPop(msg))
		{
			// Serialize message
			std::vector<uint8_t> buffer = SerializeNetworkMessage(msg.Message);

			const enet_uint32 flags = msg.Reliable ? ENET_PACKET_FLAG_RELIABLE : 0;

			std::lock_guard<std::mutex> lock(m_Mutex);

			if (!m_Client || !m_Peer)
				continue;

			ENetPacket* packet = enet_packet_create(buffer.data(), buffer.size(), flags);

			enet_peer_send(m_Peer, 0, packet);
		}

		// Flush once after processing all queued messages
		std::lock_guard<std::mutex> lock(m_Mutex);

		if (m_Client)
			enet_host_flush(m_Client);
	}

	void NetworkClient::DispatchIncomingMessages(std::stop_token stopToken)
	{
		NetworkMessage msg;

		while (m_IncomingMessages.WaitPop(msg, stopToken))
		{
			EvtMessageReceived.Trigger(msg);
		}
	}
} // namespace onion::voxel
