#include "Server.hpp"

#include <iostream>

namespace onion::voxel
{
	Server::Server() {}

	Server::~Server() {}

	void Server::Start() {}

	void Server::Stop() {}

	bool Server::IsRunning() const noexcept
	{
		return m_IsRunning.load();
	}

} // namespace onion::voxel
