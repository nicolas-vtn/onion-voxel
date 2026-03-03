#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

#include <enet/enet.h>

namespace onion::voxel
{
	class NetworkClient
	{
	  public:
		NetworkClient(const std::string& host = "127.0.0.1", int port = 7777);
		~NetworkClient();

	  public:
		void Start();
		void Stop();
		bool IsRunning() const noexcept;

		void Send(const void* data, size_t size, bool reliable = true);

	  private:
		void ListenForEvents(std::stop_token stopToken);

	  private:
		ENetHost* m_Client{nullptr};
		ENetPeer* m_Peer{nullptr};

		std::jthread m_EventThread;

		mutable std::mutex m_Mutex;

		std::string m_Host;
		int m_Port{7777};

		std::atomic_bool m_IsRunning{false};
	};
} // namespace onion::voxel
