#pragma once

#include <onion/Logger.hpp>
#include <onion/Timer.hpp>

#include <Server.hpp>

#include <shared/world/world_manager/WorldManager.hpp>

#include "ClientConfiguration.hpp"
#include <network_client/NetworkClient.hpp>
#include <renderer/Renderer.hpp>

namespace onion::voxel
{
	class Client
	{
		enum class eLogLevel
		{
			All,
			ErrorsOnly,
			None
		};

		// ----- Constructor / Destructor -----
	  public:
		Client();
		~Client();

		// ----- Public API -----
	  public:
		void Start();
		void Stop();
		void Wait();
		bool IsRunning() const noexcept;

		// ----- Getters / Setters -----
	  public:
		void SetLogLevel(eLogLevel logLevel);
		eLogLevel GetLogLevel() const;

		// ---- Client Status -----
	  private:
		glm::vec3 m_PlayerPosition;
		uint32_t m_ClientHandle = 0;

		// ----- Configuration (Client) -----
	  private:
		static inline const std::string CLIENT_VERSION = "0.1.0";
		std::filesystem::path m_ConfigFilePath = "config_client.json";
		ClientConfiguration m_Config;
		void LoadConfiguration();
		void SaveConfiguration();

		// ----- Network Client -----
	  private:
		NetworkClient m_NetworkClient;
		std::vector<EventHandle> m_NetworkClientEventHandles;
		void SubscribeToNetworkClientEvents();

		void Handle_ClientConnected(const ServerInfoMsg& message);
		void Handle_ClientDisconnected(bool disconnectedByServer);
		void Handle_NetworkMessageReceived(const NetworkMessage& message);
		void Handle_ServerInfoMessageReceived(const ServerInfoMsg& msg);
		void Handle_ChunkDataMessageReceived(const ChunkDataMsg& msg);
		void Handle_BlocksChangedMessageReceived(const BlocksChangedMsg& msg);
		void Handle_EntitySnapshotMessageReceived(const EntitySnapshotMsg& msg);

		Timer m_TimerSendPlayerInfos;
		void SendPlayerInfosToServer();

		// ----- Configuration (Server) -----
	  private:
		std::string m_ServerName;

		// ----- Event Handling -----
	  private:
		void Handle_StartSingleplayerGameRequest(const std::filesystem::path& worldPath);
		void Handle_RequestStartMultiplayerGame(const Gui::MultiplayerGameStartInfo& multiplayerGameStartInfo);
		void Handle_StopSingleplayerGameRequest(const std::filesystem::path& worldPath);

		// ----- World Manager -----
	  private:
		std::shared_ptr<WorldManager> m_WorldManager = std::make_shared<WorldManager>();
		std::vector<EventHandle> m_WorldManagerEventHandles;
		void SubscribeToWorldManagerEvents();

		void Handle_MissingChunksRequested(const std::vector<glm::ivec2>& chunkPositions);
		void Handle_BlocksChanged(const WorldManager::BlocksChangedEventArgs& args);

		// ----- Renderer -----
	  private:
		Renderer m_Renderer;
		std::vector<EventHandle> m_RendererEventHandles;
		void SubscribeToRendererEvents();

		// ----- Logger -----
	  private:
		eLogLevel m_LogLevel = eLogLevel::All;
		std::filesystem::path m_LogFile = "logs.txt";
		Logger m_Logger;

		// ----- Localhost Server -----
	  private:
		std::unique_ptr<Server> m_LocalhostServer;
	};

} // namespace onion::voxel
