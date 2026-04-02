#pragma once

#include <atomic>
#include <mutex>
#include <thread>

#include <enet/enet.h>

#include <onion/Event.hpp>
#include <onion/ThreadSafeQueue.hpp>

#include <shared/network_messages/NetworkMessages.hpp>

namespace onion::voxel
{
	class NetworkServer
	{
	  public:
		using ClientHandle = uint32_t;

		// ----- Structs -----
	  private:
		struct ClientSession
		{
			ClientHandle handle = 0;
			ENetPeer* peer = nullptr;
			std::string uuid;
			std::string ipAddress;
			bool authenticated = false;
		};

	  public:
		struct IncommingMessage
		{
			ClientHandle Sender{0};
			NetworkMessage Message{};
		};

		struct OutgoingMessage
		{
			std::vector<ClientHandle> Targets;
			NetworkMessage Message{};
			bool Reliable{true};
		};

		struct ClientConnectedEventArgs
		{
			ClientHandle Client{0};
			std::string UUID;
			std::string PlayerName;
			std::string IpAddress;
		};

		struct ClientDisconnectedEventArgs
		{
			ClientHandle Client{0};
			std::string UUID;
			std::string IpAddress;
		};

		struct MessageReceivedEventArgs
		{
			ClientHandle Sender{0};
			NetworkMessage Message{};
		};

		// ----- Constructor / Destructor -----
	  public:
		NetworkServer(uint16_t port = 7777);
		~NetworkServer();

		// ----- Public API -----
	  public:
		void Start();
		void Stop();
		bool IsRunning() const noexcept;

		void Send(ClientHandle client, NetworkMessage message, bool reliable = true);
		void Send(const std::vector<ClientHandle>& clients, NetworkMessage message, bool reliable = true);
		void Broadcast(NetworkMessage message, bool reliable = true);

		// ----- Getters / Setters -----
	  public:
		uint16_t GetServerPort() const;
		void SetServerPort(uint16_t port);

		// ----- Events -----
	  public:
		Event<const ClientConnectedEventArgs&> ClientConnected;
		Event<const ClientDisconnectedEventArgs&> ClientDisconnected;
		Event<const MessageReceivedEventArgs&> MessageReceived;

		// ---- Enet -----
	  private:
		ENetHost* m_EnetServer{nullptr};
		std::jthread m_EventThread;

		void ListenForEvents(std::stop_token stopToken);

		static std::vector<uint8_t> SerializeNetworkMessage(const NetworkMessage& message);
		void ProcessOutgoingMessages();

		// ----- Private Members -----
	  private:
		ThreadSafeQueue<IncommingMessage> m_IncomingMessages;
		ThreadSafeQueue<OutgoingMessage> m_OutgoingMessages;

		// ----- Client Management -----
	  private:
		std::mutex m_ClientMutex;
		ClientHandle m_NextClientHandle{1};
		ClientHandle GenerateClientHandle();
		std::unordered_map<ENetPeer*, ClientSession> m_PeerToSession;
		std::unordered_map<ClientHandle, ENetPeer*> m_HandleToPeer;

		// ----- Message Reception and Dispatching -----
	  private:
		std::jthread m_DispatchIncomingMessagesThread;
		void DispatchIncomingMessages(std::stop_token stopToken);

		// ----- States -----
	  private:
		mutable std::mutex m_Mutex;
		uint16_t m_Port{7777};
		std::atomic_bool m_IsRunning{false};
	};
} // namespace onion::voxel
