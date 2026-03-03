#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

#include <enet/enet.h>

#include <shared/network_messages/NetworkMessages.hpp>
#include <shared/thread_safe_queue/ThreadSafeQueue.hpp>

namespace onion::voxel
{
	class NetworkClient
	{
		// ----- Constructor / Destructor -----
	  public:
		NetworkClient(const std::string& host = "127.0.0.1", int port = 7777);
		~NetworkClient();

		// ----- Public API -----
	  public:
		void Start();
		void Stop();
		bool IsRunning() const noexcept;

		template <typename T> inline void SendNetworkMessage(MessageHeader::eType type, const T& message, bool reliable)
		{
			std::ostringstream stream(std::ios::binary);
			cereal::BinaryOutputArchive archive(stream);

			MessageHeader header = BuildMessageHeader(type);

			archive(header);
			archive(message);

			std::string buffer = stream.str();

			ENetPacket* packet =
				enet_packet_create(buffer.data(), buffer.size(), reliable ? ENET_PACKET_FLAG_RELIABLE : 0);

			{
				std::lock_guard<std::mutex> lock(m_Mutex);

				if (!m_Peer)
					return;

				enet_peer_send(m_Peer, 0, packet);
				enet_host_flush(m_Client);
			}
		}

		// ----- Getters / Setters -----
	  public:
		bool TryPopMessage(NetworkMessage& out);

	  private:
		MessageHeader BuildMessageHeader(MessageHeader::eType type) const;
		std::jthread m_EventThread;
		void ListenForEvents(std::stop_token stopToken);

		// ------ Public Members ------
	  private:
		ThreadSafeQueue<NetworkMessage> m_IncomingMessages;

	  private:
		mutable std::mutex m_Mutex;
		ENetHost* m_Client{nullptr};
		ENetPeer* m_Peer{nullptr};

		std::string m_Host;
		int m_Port{7777};

		std::atomic_bool m_IsRunning{false};
	};

} // namespace onion::voxel
