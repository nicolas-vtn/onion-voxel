#include "Server.hpp"

#include <iostream>

namespace onion::voxel
{
	Server::Server() {}

	Server::~Server()
	{
		Stop();
	}

	void Server::Start()
	{
		m_NetworkServer.Start();
		m_IsRunning.store(true);
	}

	void Server::Stop()
	{
		m_NetworkServer.Stop();
		m_IsRunning.store(false);
	}

	bool Server::IsRunning() const noexcept
	{
		return m_IsRunning.load();
	}

} // namespace onion::voxel
