#include "Client.hpp"

#include <iostream>

namespace onion::voxel
{
	Client::Client() : m_Logger(m_LogFile.string())
	{
		SetLogLevel(m_LogLevel);
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
		// Starts a Server on Localhost and connect to it with the Client.

		// Sets Renderer UI to InGame UI.
		m_Renderer.SetRenderState(Renderer::eRenderState::InGame);
	}

	void Client::SubscribeToRendererEvents()
	{
		m_RendererEventHandles.push_back(m_Renderer.RequestStartSingleplayerGame.Subscribe(
			[this](const std::filesystem::path& worldPath) { Handle_StartSingleplayerGameRequest(worldPath); }));
	}

} // namespace onion::voxel
