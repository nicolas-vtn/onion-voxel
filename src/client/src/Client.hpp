#pragma once

#include <onion/Logger.hpp>

#include <Server.hpp>

#include <enet/enet.h>

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

		// ----- Event Handling -----
	  private:
		void Handle_StartSingleplayerGameRequest(const std::filesystem::path& worldPath);
		void Handle_StopSingleplayerGameRequest(const std::filesystem::path& worldPath);

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

		// ----- Localhost Server -----
	  private:
		std::unique_ptr<Server> m_LocalhostServer;
	};

} // namespace onion::voxel
