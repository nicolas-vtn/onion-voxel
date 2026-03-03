#pragma once

#include <atomic>
#include <mutex>
#include <thread>

#include <enet/enet.h>

#include <shared/network_messages/NetworkMessages.hpp>
#include <shared/thread_safe_queue/ThreadSafeQueue.hpp>

namespace onion::voxel
{
	class NetworkServer
	{
		// ----- Constructor / Destructor -----
	  public:
		NetworkServer(int port = 7777);
		~NetworkServer();

		// ----- Public API -----
	  public:
		void Start();
		void Stop();
		bool IsRunning() const noexcept;

		// ----- Getters / Setters -----
	  public:
		bool TryPopMessage(NetworkMessage& out);

		// ---- Enet -----
	  private:
		ENetHost* m_EnetServer{nullptr};
		std::jthread m_EventThread;

		void ListenForEvents(std::stop_token stopToken);

		// ----- Private Members -----
	  public:
		ThreadSafeQueue<NetworkMessage> m_IncomingMessages;

		// ----- States -----
	  private:
		mutable std::mutex m_Mutex;
		int m_Port{7777};
		std::atomic_bool m_IsRunning{false};
	};
} // namespace onion::voxel
