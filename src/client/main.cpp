#include <Client.hpp>

#include <atomic>
#include <chrono>
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

	return 0;
}
