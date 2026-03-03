#pragma once

#include <atomic>
#include <mutex>
#include <thread>

#include <enet/enet.h>

#include <onion/Event.hpp>
#include <shared/network_messages/NetworkMessages.hpp>
#include <shared/thread_safe_queue/ThreadSafeQueue.hpp>

namespace onion::voxel
{
	class NetworkServer
	{
	  public:
		using ClientHandle = uint32_t;

		// ----- Structs -----
	  private:
		struct ClientSession
		{
			ClientHandle handle = 0;
			ENetPeer* peer = nullptr;
			std::string uuid;
			bool authenticated = false;
		};

	  public:
		struct ServerEvent
		{
			ClientHandle Sender;
			NetworkMessage Message;
		};

		// ----- Constructor / Destructor -----
	  public:
		NetworkServer(int port = 7777);
		~NetworkServer();

		// ----- Public API -----
	  public:
		void Start();
		void Stop();
		bool IsRunning() const noexcept;

		void Send(ClientHandle client, NetworkMessage message, bool reliable = true);
		void Send(const std::vector<ClientHandle>& clients, NetworkMessage message, bool reliable = true);
		void Broadcast(NetworkMessage message, bool reliable = true);

		// ----- Getters / Setters -----
	  public:
		bool TryPopMessage(ServerEvent& out);

		// ----- Events -----
	  public:
		Event<ClientHandle> ClientConnected;
		Event<ClientHandle> ClientDisconnected;

		// ---- Enet -----
	  private:
		ENetHost* m_EnetServer{nullptr};
		std::jthread m_EventThread;

		void ListenForEvents(std::stop_token stopToken);

		// ----- Private Members -----
	  private:
		ThreadSafeQueue<ServerEvent> m_IncomingMessages;

		// ----- Client Management -----
	  private:
		std::mutex m_ClientMutex;
		ClientHandle m_NextClientHandle{1};
		ClientHandle GenerateClientHandle();
		std::unordered_map<ENetPeer*, ClientSession> m_PeerToSession;
		std::unordered_map<ClientHandle, ENetPeer*> m_HandleToPeer;

		// ----- States -----
	  private:
		mutable std::mutex m_Mutex;
		int m_Port{7777};
		std::atomic_bool m_IsRunning{false};
	};
} // namespace onion::voxel
