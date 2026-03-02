#pragma once

#include <renderer/Renderer.hpp>

#include <onion/Logger.hpp>

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

	  public:
		Client();
		~Client();

	  public:
		void Start();
		void Stop();
		void Wait();
		bool IsRunning() const noexcept;

		void SetLogLevel(eLogLevel logLevel);
		eLogLevel GetLogLevel() const;

	  public:
	  private:
		Renderer m_Renderer;

		eLogLevel m_LogLevel = eLogLevel::All;
		std::filesystem::path m_LogFile = "logs.txt";
		Logger m_Logger;
	};

} // namespace onion::voxel
