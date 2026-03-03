#pragma once

#include <atomic>
#include <mutex>
#include <thread>

#include <enet/enet.h>

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

		// ---- Enet -----
	  private:
		ENetHost* m_EnetServer{nullptr};
		std::jthread m_EventThread;
		void ListenForEvents(std::stop_token stopToken);

		// ----- States -----
	  private:
		mutable std::mutex m_Mutex;
		int m_Port{7777};
		std::atomic_bool m_IsRunning{false};
	};
} // namespace onion::voxel
