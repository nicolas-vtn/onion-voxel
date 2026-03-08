#pragma once

#include <atomic>

#include "network_server/NetworkServer.hpp"
#include <shared/network_messages/Serializer.hpp>

#include "world_generator/WorldGenerator.hpp"
#include <shared/world/world_manager/WorldManager.hpp>

namespace onion::voxel
{
	class Server
	{
		// ----- Constructor / Destructor -----
	  public:
		Server();
		~Server();

		// ----- Public API -----
	  public:
		void Start();
		void Stop();

		bool IsRunning() const noexcept;

		// ----- States -----
	  private:
		std::atomic_bool m_IsRunning{false};

		// ----- Network Server -----
	  private:
		NetworkServer m_NetworkServer;
		std::vector<EventHandle> m_NetworkServerEventHandles;
		void SubscribeToNetworkServerEvents();

		void Handle_ClientConnected(const NetworkServer::ClientConnectedEventArgs& args);
		void Handle_ClientDisconnected(const NetworkServer::ClientDisconnectedEventArgs& args);
		void Handle_NetworkMessageReceived(const NetworkServer::MessageReceivedEventArgs& args);

		void Handle_ClientInfoMsgReceived(const NetworkServer::MessageReceivedEventArgs& args,
										  const ClientInfoMsg& msg);

		// ----- World Generation / Management -----
	  private:
		std::shared_ptr<WorldManager> m_WorldManager;
		std::vector<EventHandle> m_WorldManagerEventHandles;
		void SubscribeToWorldManagerEvents();

		void Handle_ChunkAdded(const std::shared_ptr<Chunk>& chunk);
		void Handle_ChunkRemoved(const std::shared_ptr<Chunk>& chunk);

		WorldGenerator m_WorldGenerator;

		// ----- Event Handlers -----
	  private:
	};
} // namespace onion::voxel
