#include <onion/Logger.hpp>

#include "Server.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <thread>

namespace
{
	std::atomic_bool g_stopRequested{false};

	void SignalHandler(int)
	{
		g_stopRequested.store(true);
	}
} // namespace

int main()
{
	onion::Logger logger(std::filesystem::path("server.log"), std::filesystem::path("server.log"), "SERVER");

	std::cout << "\n --- ONION VOXEL SERVER ---" << std::endl;

	std::signal(SIGINT, SignalHandler);

	{
		onion::voxel::Server server;

		server.Start();

		while (!g_stopRequested.load() && server.IsRunning())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}

		server.Stop();
	}

	std::cout << "\n --- Server Destroyed Successfully --- \n";

	std::cout << "Press ENTER to exit..." << std::endl;
	std::cin.get();

	return 0;
}
