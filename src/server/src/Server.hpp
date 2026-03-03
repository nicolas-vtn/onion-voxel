#pragma once

#include <atomic>

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
	};
}; // namespace onion::voxel
