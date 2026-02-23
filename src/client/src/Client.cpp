#include "Client.hpp"

#include <iostream>

namespace onion::voxel
{
	void Client::SayHello()
	{
		std::cout << "Hello, I'm the Client" << std::endl;
	}

	void Client::Start()
	{
		std::cout << "Start Client" << std::endl;
		m_Renderer.Start();
	}

	void Client::Stop()
	{
		std::cout << "Stop Client" << std::endl;
		m_Renderer.Stop();
	}

	void Client::Wait() {}

	bool Client::IsRunning() const noexcept
	{
		bool isRendererRunning = m_Renderer.IsRunning();
		return isRendererRunning;
	}

} // namespace onion::voxel
