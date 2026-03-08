#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

#include <enet/enet.h>

#include <onion/Event.hpp>

#include <shared/network_messages/NetworkMessages.hpp>
#include <shared/thread_safe_queue/ThreadSafeQueue.hpp>

namespace onion::voxel
{
	class NetworkClient
	{
		using ClientHandle = uint32_t;

		// ----- Constructor / Destructor -----
	  public:
		NetworkClient(const std::string& host = "127.0.0.1", int port = 7777);
		~NetworkClient();

		// ----- Public API -----
	  public:
		void Start();
		void Stop();
		bool IsRunning() const noexcept;

		void Send(NetworkMessage message, bool reliable = true);
		// ----- Events -----
	  public:
		Event<const NetworkMessage&> MessageReceived;

		// ----- Private Structs -----
	  private:
		struct OutgoingMessage
		{
			NetworkMessage Message;
			bool Reliable;
		};

		// ------ Private Members ------
	  private:
		std::atomic<ClientHandle> m_ClientHandle{0};

		// ----- Message Queues -----
	  private:
		ThreadSafeQueue<NetworkMessage> m_IncomingMessages;
		ThreadSafeQueue<OutgoingMessage> m_OutgoingMessages;

		// ----- Enet -----
	  private:
		mutable std::mutex m_Mutex;
		ENetHost* m_Client{nullptr};
		ENetPeer* m_Peer{nullptr};
		std::string m_Host;
		int m_Port{7777};

		std::jthread m_EventThread;
		void ListenForEvents(std::stop_token stopToken);
		std::atomic_bool m_IsRunning{false};

		std::vector<uint8_t> SerializeNetworkMessage(const NetworkMessage& message);
		void ProcessOutgoingMessages();

		// ----- Incoming Message Handling -----
	  private:
		std::jthread m_DispatchIncomingMessagesThread;
		void DispatchIncomingMessages(std::stop_token stopToken);
	};

} // namespace onion::voxel
