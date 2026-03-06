#include "Client.hpp"

#include <iostream>

namespace onion::voxel
{
	Client::Client() : m_Logger(m_LogFile.string()), m_Renderer(m_WorldManager)
	{
		LoadConfiguration();

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

	void Client::LoadConfiguration()
	{
		m_Config.Load(m_ConfigFilePath);
	}

	void Client::SaveConfiguration()
	{
		m_Config.Save(m_ConfigFilePath);
	}

	void Client::Handle_StartSingleplayerGameRequest(const std::filesystem::path& worldPath)
	{
		// Starts a Server on Localhost
		if (m_LocalhostServer == nullptr)
		{
			m_LocalhostServer = std::make_unique<Server>();
			m_LocalhostServer->Start();

			// DEBUG ONLY : Add A Chunk
			// For testing, add a chunk at (0, 0)
			std::shared_ptr<Chunk> chunk = std::make_shared<Chunk>(glm::ivec2(0, 0));

			// Set some blocks in the chunk for testing
			Block grass{BlockId::Grass};
			chunk->SetBlock(glm::ivec3(0, 0, 0), grass);

			Block dirt{BlockId::Dirt};
			chunk->SetBlock(glm::ivec3(2, 0, 0), dirt);

			Block stone{BlockId::Stone};
			stone.m_Facing = Block::Orientation::South;
			stone.m_Top = Block::Orientation::Down;
			stone.m_IsRotated = true;
			chunk->SetBlock(glm::ivec3(4, 0, 0), stone);

			Block glass{BlockId::Glass};
			chunk->SetBlock(glm::ivec3(6, 0, 0), glass);

			m_WorldManager->AddChunk(chunk);
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

		// Sets Renderer UI to InGame UI.
		m_Renderer.SetRenderState(Renderer::eRenderState::InGame);

		// Sends a message to Server
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		ClientInfoMsg clientInfoMsg;
		clientInfoMsg.Username = m_Config.clientData.Username;
		clientInfoMsg.UUID = m_Config.clientData.UUID;

		m_NetworkClient.Send(std::move(clientInfoMsg), true);
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

		m_Renderer.SetRenderState(Renderer::eRenderState::Menu);
	}

	void Client::SubscribeToRendererEvents()
	{
		m_RendererEventHandles.push_back(m_Renderer.RequestStartSingleplayerGame.Subscribe(
			[this](const std::filesystem::path& worldPath) { Handle_StartSingleplayerGameRequest(worldPath); }));

		m_RendererEventHandles.push_back(
			m_Renderer.RequestQuitToMainMenu.Subscribe([this](bool quit) { Handle_StopSingleplayerGameRequest(""); }));
	}

} // namespace onion::voxel
