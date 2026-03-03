#pragma once

#include <onion/Logger.hpp>

#include "../../server/src/Server.hpp"

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
