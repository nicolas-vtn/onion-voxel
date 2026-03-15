#include <Client.hpp>

#include <atomic>
#include <chrono>
#include <conio.h>
#include <csignal>
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
	std::cout << "\n --- ONION VOXEL ---" << std::endl;

	std::signal(SIGINT, SignalHandler);

	{
		onion::voxel::Client client;

		client.Start();

		while (!g_stopRequested.load() && client.IsRunning())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		client.Stop();
		client.Wait();
	}

	std::cout << "\n --- Client Destroyed Successfully --- \n";

	std::cout << "Press any key to exit..." << std::endl;
	while (!_kbhit())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	return 0;
}
