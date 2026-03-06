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
			chunk->SetBlock(glm::ivec3(4, 0, 0), stone);

			Block glass{BlockId::Glass};
			chunk->SetBlock(glm::ivec3(6, 0, 0), glass);

			Block oakLeaves{BlockId::OakLeaves};
			chunk->SetBlock(glm::ivec3(8, 0, 0), oakLeaves);

			Block oakLog0{BlockId::OakLog};
			oakLog0.m_Facing = Block::Orientation::North;
			oakLog0.m_Top = Block::Orientation::Up;
			chunk->SetBlock(glm::ivec3(10, 0, 0), oakLog0);

			Block oakLogHorizontal1{BlockId::OakLog};
			oakLogHorizontal1.m_Facing = Block::Orientation::Up;
			oakLogHorizontal1.m_Top = Block::Orientation::North;
			chunk->SetBlock(glm::ivec3(12, 0, 0), oakLogHorizontal1);

			Block oakLogHorizontal2{BlockId::OakLog};
			oakLogHorizontal2.m_Facing = Block::Orientation::East;
			oakLogHorizontal2.m_Top = Block::Orientation::Up;
			chunk->SetBlock(glm::ivec3(14, 0, 0), oakLogHorizontal2);

			Block furnaceNorth{BlockId::Furnace};
			furnaceNorth.m_Facing = Block::Orientation::North;
			furnaceNorth.m_Top = Block::Orientation::Up;
			chunk->SetBlock(glm::ivec3(0, 0, 4), furnaceNorth);

			Block furnaceSouth{BlockId::Furnace};
			furnaceSouth.m_Facing = Block::Orientation::South;
			furnaceSouth.m_Top = Block::Orientation::Up;
			chunk->SetBlock(glm::ivec3(2, 0, 4), furnaceSouth);

			Block furnaceEast{BlockId::Furnace};
			furnaceEast.m_Facing = Block::Orientation::East;
			furnaceEast.m_Top = Block::Orientation::Up;
			chunk->SetBlock(glm::ivec3(4, 0, 4), furnaceEast);

			Block furnaceWest{BlockId::Furnace};
			furnaceWest.m_Facing = Block::Orientation::West;
			furnaceWest.m_Top = Block::Orientation::Up;
			chunk->SetBlock(glm::ivec3(6, 0, 4), furnaceWest);

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
