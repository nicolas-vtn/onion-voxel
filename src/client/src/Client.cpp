#include "Client.hpp"

#include <iostream>

namespace onion::voxel
{
	Client::Client() : m_Logger(m_LogFile.string())
	{
		SetLogLevel(m_LogLevel);

		SubscribeToRendererEvents();
	}

	Client::~Client() {}

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

	void Client::SetLogLevel(eLogLevel logLevel)
	{
		m_LogLevel = logLevel;

		switch (logLevel)
		{
			case eLogLevel::All:
				m_Logger.SetLogInfos(true);
				m_Logger.SetLogErrors(true);
				break;
			case eLogLevel::ErrorsOnly:
				m_Logger.SetLogInfos(false);
				m_Logger.SetLogErrors(true);
				break;
			case eLogLevel::None:
				m_Logger.SetLogInfos(false);
				m_Logger.SetLogErrors(false);
				break;
		}
	}

	Client::eLogLevel Client::GetLogLevel() const
	{
		return m_LogLevel;
	}

	void Client::Handle_StartSingleplayerGameRequest(const std::filesystem::path& worldPath)
	{
		// Starts a Server on Localhost
		if (m_LocalhostServer == nullptr)
		{
			m_LocalhostServer = std::make_unique<Server>();
			m_LocalhostServer->Start();
		}
		else
		{
			throw std::runtime_error("Localhost Server is already running");
		}

		// Connects to Localhost Server
		if (!m_NetworkClient.IsRunning())
		{
			m_NetworkClient.Start();
		}
		else
		{
			throw std::runtime_error("Network Client is already running");
		}

		// Sends a message to Server
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		m_NetworkClient.SendNetworkMessage(MessageHeader::eType::ClientInfo, ClientInfoMsg{"Hello from Client!"}, true);

		// Sets Renderer UI to InGame UI.
		m_Renderer.SetRenderState(Renderer::eRenderState::InGame);
	}

	void Client::Handle_StopSingleplayerGameRequest(const std::filesystem::path& worldPath)
	{
		if (m_NetworkClient.IsRunning())
		{
			m_NetworkClient.Stop();
		}

		if (m_LocalhostServer != nullptr)
		{
			m_LocalhostServer->Stop();
			m_LocalhostServer.reset();
		}
	}

	void Client::SubscribeToRendererEvents()
	{
		m_RendererEventHandles.push_back(m_Renderer.RequestStartSingleplayerGame.Subscribe(
			[this](const std::filesystem::path& worldPath) { Handle_StartSingleplayerGameRequest(worldPath); }));
	}

} // namespace onion::voxel
