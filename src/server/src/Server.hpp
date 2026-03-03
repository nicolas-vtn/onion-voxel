#pragma once

#include <atomic>

#include "network_server/NetworkServer.hpp"

namespace onion::voxel
{
	class Server
	{
		// ----- Constructor / Destructor -----
	  public:
		Server();
		~Server();

		// ----- Public API -----
	  public:
		void Start();
		void Stop();

		bool IsRunning() const noexcept;

		// ----- States -----
	  private:
		std::atomic_bool m_IsRunning{false};

		// ----- Network Server -----
	  private:
		NetworkServer m_NetworkServer;
	};
} // namespace onion::voxel
