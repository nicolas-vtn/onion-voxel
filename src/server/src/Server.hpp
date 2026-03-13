#pragma once

#include <atomic>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include "ServerConfiguration.hpp"
#include "network_server/NetworkServer.hpp"

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

		// ----- Configuration (Server) -----
	  private:
		static inline const std::string SERVER_VERSION = "0.1.0";
		std::filesystem::path m_ConfigFilePath = "config_server.json";
		ServerConfiguration m_Config;
		void LoadConfiguration();
		void SaveConfiguration();

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
		void Handle_PlayerInfoMsgReceived(const NetworkServer::MessageReceivedEventArgs& args,
										  const PlayerInfoMsg& msg);
		void Handle_PlayerRequestChunksMsgReceived(const NetworkServer::MessageReceivedEventArgs& args,
												   const RequestChunksMsg& msg);
		void Handle_BlocksChangedMsgReceived(const NetworkServer::MessageReceivedEventArgs& args,
											 const BlocksChangedMsg& msg);

		// ----- World Manager / Generation -----
	  private:
		std::shared_ptr<WorldManager> m_WorldManager;
		std::vector<EventHandle> m_WorldManagerEventHandles;
		void SubscribeToWorldManagerEvents();

		void Handle_ChunkAdded(const std::shared_ptr<Chunk>& chunk);
		void Handle_ChunkRemoved(const std::shared_ptr<Chunk>& chunk);
		void Handle_BlocksChanged(const WorldManager::BlocksChangedEventArgs& args);
		void Handle_MissingChunksRequested(const std::vector<glm::ivec2>& missingChunkPositions);

		WorldGenerator m_WorldGenerator;

		// ----- Players -----
	  private:
		struct PlayerInfo
		{
			std::string Username;
			std::string UUID;
			uint32_t ClientHandle;
			glm::vec3 Position{99999.0f, 99999.0f, 99999.0f};
		};

		mutable std::shared_mutex m_MutexPlayers;
		std::unordered_map<uint32_t, PlayerInfo> m_Players;
		std::unordered_map<uint32_t, glm::ivec2>
			m_PlayerChunkPositions; // The chunk positions of the chunks that each player is currently in

		void AddOrUpdatePlayer(const PlayerInfo& playerInfo);
		void UpdatePlayerPosition(uint32_t clientHandle, const glm::vec3& newPosition);
		void RemovePlayer(uint32_t clientHandle);
		PlayerInfo GetPlayerInfo(uint32_t clientHandle) const;
	};
} // namespace onion::voxel
