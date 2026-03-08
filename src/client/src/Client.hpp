#pragma once

#include <onion/Logger.hpp>

#include <Server.hpp>

#include <enet/enet.h>

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

		// ----- Configuration -----
	  private:
		static inline const std::string GAME_VERSION = "0.1.0";
		std::filesystem::path m_ConfigFilePath = "config.json";
		ClientConfiguration m_Config;
		void LoadConfiguration();
		void SaveConfiguration();

		// ----- Event Handling -----
	  private:
		void Handle_StartSingleplayerGameRequest(const std::filesystem::path& worldPath);
		void Handle_StopSingleplayerGameRequest(const std::filesystem::path& worldPath);

		// ----- World Manager -----
	  private:
		std::shared_ptr<WorldManager> m_WorldManager = std::make_shared<WorldManager>();

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

		// ----- Network Client -----
	  private:
		NetworkClient m_NetworkClient;
		std::vector<EventHandle> m_NetworkClientEventHandles;
		void SubscribeToNetworkClientEvents();

		void Handle_NetworkMessageReceived(const NetworkMessage& message);
		void Handle_ServerInfoMessageReceived(const ServerInfoMsg& msg);
		void Handle_ChunkDataMessageReceived(const ChunkDataMsg& msg);

		// ----- Localhost Server -----
	  private:
		std::unique_ptr<Server> m_LocalhostServer;
	};

} // namespace onion::voxel
