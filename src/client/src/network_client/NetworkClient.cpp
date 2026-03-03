#include "NetworkClient.hpp"

#include <iostream>
#include <stdexcept>

#include <shared/init_enet_once/InitEnetOnce.hpp>

namespace onion::voxel
{
	NetworkClient::NetworkClient(const std::string& host, int port) : m_Host(host), m_Port(port)
	{
		InitEnetOnce::Init();
	}

	NetworkClient::~NetworkClient()
	{
		Stop();
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

		std::lock_guard<std::mutex> lock(m_Mutex);

		if (m_Peer)
		{
			enet_peer_disconnect(m_Peer, 0);
			m_Peer = nullptr;
		}

		if (m_Client)
		{
			enet_host_destroy(m_Client);
			m_Client = nullptr;
		}

		m_IsRunning.store(false);
	}

	bool NetworkClient::IsRunning() const noexcept
	{
		return m_IsRunning.load();
	}

	bool NetworkClient::TryPopMessage(NetworkMessage& out)
	{
		return m_IncomingMessages.TryPop(out);
	}

	MessageHeader NetworkClient::BuildMessageHeader(MessageHeader::eType type) const
	{
		return MessageHeader{type, "ClientId"};
	}

	void NetworkClient::ListenForEvents(std::stop_token stopToken)
	{
		ENetEvent event;

		while (!stopToken.stop_requested())
		{
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

								m_IncomingMessages.Push(DeserializeMessage(archive, header.Type));
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
						m_Peer = nullptr;
						return;

					default:
						break;
				}
			}
		}
	}
} // namespace onion::voxel
