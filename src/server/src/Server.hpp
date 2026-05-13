#pragma once

#include <atomic>
#include <chrono>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include <onion/Timer.hpp>

#include "ServerConfiguration.hpp"
#include "network_server/NetworkServer.hpp"

#include <shared/chat_history/ChatHistory.hpp>
#include <shared/physics/PhysicsEngine.hpp>
#include <shared/world/world_manager/WorldManager.hpp>

namespace onion::voxel
{
	class Server
	{
		// ----- Constructor / Destructor -----
	  public:
		Server();
		Server(const ServerConfiguration& config);
		~Server();

		// ----- Public API -----
	  public:
		void Start();
		void Stop();

		bool IsRunning() const noexcept;

		// ----- Getters / Setters -----
	  public:
		void SetChunkLoadingDistance(uint8_t distance);

		// ----- Configuration (Server) -----
	  private:
		static inline const std::string SERVER_VERSION = "0.1.0";
		std::filesystem::path m_ConfigFilePath = "config_server.json";
		ServerConfiguration m_Config;
		void Initialize();
		void LoadConfiguration();
		void SaveConfiguration();

		// ----- States -----
	  private:
		std::atomic_bool m_IsRunning{false};

		// ----- Network Server -----
	  private:
		NetworkServer m_NetworkServer;
		void UpdateMOTD();
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
		void Handle_ItemDroppedMsgReceived(const NetworkServer::MessageReceivedEventArgs& args,
										   const ItemDroppedMsg& msg);
		void Handle_ChatMsgReceived(const NetworkServer::MessageReceivedEventArgs& args, const ChatMsg& msg);

		// ----- World Manager / Generation -----
	  private:
		std::shared_ptr<WorldManager> m_WorldManager;
		std::vector<EventHandle> m_WorldManagerEventHandles;
		void SubscribeToWorldManagerEvents();

		void Handle_ChunkAdded(const std::shared_ptr<Chunk>& chunk);
		void Handle_ChunkRemoved(const std::shared_ptr<Chunk>& chunk);
		void Handle_BlocksChanged(const WorldManager::BlocksChangedEventArgs& args);

		// ----- Physics Engine -----
	  private:
		std::unique_ptr<PhysicsEngine> m_PhysicsEngine;

		// ----- Chat History -----
	  private:
		ChatHistory m_ChatHistory;

		// ----- Timer Send Events -----
	  private:
		Timer m_TimerSendEvents;
		void Handle_TimerSendEvents();

		// ----- Timer Physics Tick -----
	  private:
		Timer m_TimerPhysicsTick;
		std::chrono::steady_clock::time_point m_LastPhysicsTick;
		void Handle_TimerPhysicsTick();

		// ----- Players -----
	  private:
		struct PlayerInfo
		{
			std::string PlayerName;
			std::string UUID;
			uint32_t ClientHandle = 0;
		};

		mutable std::shared_mutex m_MutexPlayers;
		std::unordered_map<uint32_t, PlayerInfo> m_ClientHandleToPlayerInfo;
		std::unordered_map<std::string, PlayerInfo> m_UUIDToPlayerInfo;

		void AddPlayer(const PlayerInfo& playerInfo);
		void RemovePlayer(const std::string& uuid);
		std::shared_ptr<Player> GetPlayer(const uint32_t clientHandle);
		std::optional<PlayerInfo> GetPlayerInfo(const std::string uuid);
	};
} // namespace onion::voxel
