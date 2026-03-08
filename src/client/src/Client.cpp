#include "Client.hpp"

#include <iostream>

#include <shared/network_messages/Serializer.hpp>

namespace onion::voxel
{
	Client::Client() : m_Logger(m_LogFile.string()), m_Renderer(m_WorldManager)
	{
		LoadConfiguration();

		SetLogLevel(m_LogLevel);

		SubscribeToRendererEvents();
		SubscribeToNetworkClientEvents();

		m_TimerSendPlayerInfos.setTimeoutFunction([this]() { SendPlayerInfosToServer(); });
		m_TimerSendPlayerInfos.setElapsedPeriod(std::chrono::milliseconds(500));
		m_TimerSendPlayerInfos.setRepeat(true);
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
		}
		else
		{
			throw std::runtime_error("Localhost Server is already running");
		}

		// Connects to Localhost Server
		if (!m_NetworkClient.IsRunning())
		{
			m_NetworkClient.Start();
			m_TimerSendPlayerInfos.Start();
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
			m_TimerSendPlayerInfos.Stop();
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

	void Client::SubscribeToNetworkClientEvents()
	{
		m_NetworkClientEventHandles.push_back(m_NetworkClient.Connected.Subscribe(
			[this](const ServerInfoMsg& message) { Handle_ClientConnected(message); }));

		m_NetworkClientEventHandles.push_back(m_NetworkClient.Disconnected.Subscribe(
			[this](bool disconnectedByServer) { Handle_ClientDisconnected(disconnectedByServer); }));

		m_NetworkClientEventHandles.push_back(m_NetworkClient.MessageReceived.Subscribe(
			[this](const NetworkMessage& message) { Handle_NetworkMessageReceived(message); }));
	}

	void Client::Handle_ClientConnected(const ServerInfoMsg& message)
	{
		Handle_ServerInfoMessageReceived(message);
	}

	void Client::Handle_ClientDisconnected(bool disconnectedByServer)
	{
		m_Renderer.SetServerInfo(nullptr);
	}

	void Client::Handle_NetworkMessageReceived(const NetworkMessage& message)
	{
		std::visit(
			[this](const auto& msg)
			{
				using T = std::decay_t<decltype(msg)>;
				if constexpr (std::is_same_v<T, ServerInfoMsg>)
				{
					Handle_ServerInfoMessageReceived(msg);
				}
				else if constexpr (std::is_same_v<T, ChunkDataMsg>)
				{
					Handle_ChunkDataMessageReceived(msg);
				}
			},
			message);
	}

	void Client::Handle_ServerInfoMessageReceived(const ServerInfoMsg& msg)
	{
		std::cout << "Received ServerInfoMsg: ServerName=" << msg.ServerName << ", ClientHandle=" << msg.ClientHandle
				  << std::endl;

		m_ClientHandle = msg.ClientHandle;
		m_ServerName = msg.ServerName;

		std::shared_ptr<ServerInfo> serverInfo = std::make_shared<ServerInfo>();
		serverInfo->Name = msg.ServerName;
		serverInfo->Address = m_NetworkClient.GetRemoteHost();
		serverInfo->Port = m_NetworkClient.GetRemotePort();
		serverInfo->SimulationDistance = msg.SimulationDistance;

		m_Renderer.SetServerInfo(serverInfo);
	}

	void Client::Handle_ChunkDataMessageReceived(const ChunkDataMsg& msg)
	{
		std::shared_ptr<Chunk> chunk = Serializer::DeserializeChunk(msg);
		m_WorldManager->AddChunk(chunk);
	}

	void Client::SendPlayerInfosToServer()
	{
		PlayerInfoMsg playerInfoMsg;
		playerInfoMsg.Username = m_Config.clientData.Username;
		playerInfoMsg.UUID = m_Config.clientData.UUID;
		playerInfoMsg.Position = m_Renderer.GetPlayerPosition();

		m_NetworkClient.Send(std::move(playerInfoMsg), false);
	}

} // namespace onion::voxel
