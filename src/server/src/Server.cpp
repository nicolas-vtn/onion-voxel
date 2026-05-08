#include "Server.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <shared/data_transfer_objects/serializer/SerializerDTO.hpp>
#include <shared/entities/entity/block_entity/BlockEntity.hpp>
#include <shared/network_messages/item_picked_up_msg/ItemPickedUpMsg.hpp>
#include <shared/utils/Utils.hpp>

namespace onion::voxel
{
	Server::Server()
	{
		LoadConfiguration();

		// Create world if it doesn't exist
		m_Config.serverData.WorldDirectory = Utils::GetExecutableDirectory() / "world";
		const std::filesystem::path& worldDir = m_Config.serverData.WorldDirectory;
		if (!std::filesystem::exists(worldDir))
		{
			WorldInfos infos;
			infos.Seed = m_Config.serverData.Seed;
			infos.Name = m_Config.serverData.ServerName;
			infos.CreationDate = DateTime::UtcNow();
			infos.WorldGenerationType =
				static_cast<WorldGenerator::eWorldGenerationType>(m_Config.serverData.WorldGenerationType);
			WorldSave::CreateWorld(worldDir, infos);
		}

		Initialize();

		// ---- Set MOTD Data ----
		UpdateMOTD();
	}

	Server::Server(const ServerConfiguration& config) : m_Config(config)
	{
		Initialize();
	}

	Server::~Server()
	{
		m_NetworkServerEventHandles.clear();
		m_WorldManagerEventHandles.clear();

		m_NetworkServer.Stop();

		m_WorldManager->ClearWorld();

		Stop();
	}

	void Server::Start()
	{
		m_TimerSendEvents.Start();
		m_TimerPhysicsTick.Start();
		m_NetworkServer.Start();
		m_IsRunning.store(true);
	}

	void Server::Stop()
	{
		m_TimerSendEvents.Stop();
		m_TimerPhysicsTick.Stop();
		m_NetworkServer.Stop();
		m_IsRunning.store(false);
	}

	bool Server::IsRunning() const noexcept
	{
		return m_IsRunning.load();
	}

	void Server::SetChunkLoadingDistance(uint8_t distance)
	{
		m_Config.serverData.SimulationDistance = distance;

		m_WorldManager->SetChunkLoadingDistance(distance);
		m_WorldManager->SetChunkPersistanceDistance(distance);

		// Broadcast new simulation distance to all clients
		ServerInfoMsg srvInfoMsg;
		srvInfoMsg.ServerName = m_Config.serverData.ServerName;
		srvInfoMsg.SimulationDistance = m_Config.serverData.SimulationDistance;

		// Sends the ServerInfoMsg to the newly connected client
		m_NetworkServer.Broadcast(srvInfoMsg);
	}

	void Server::Initialize()
	{
		m_NetworkServer.SetServerPort(m_Config.serverData.Port);

		m_WorldManager = std::make_shared<WorldManager>(m_Config.serverData.WorldDirectory, false);

		// Apply Configuration
		m_WorldManager->SetChunkPersistanceDistance(m_Config.serverData.SimulationDistance);
		m_WorldManager->SetChunkLoadingDistance(m_Config.serverData.SimulationDistance);

		m_PhysicsEngine = std::make_unique<PhysicsEngine>(*m_WorldManager);

		SubscribeToNetworkServerEvents();
		SubscribeToWorldManagerEvents();

		// Setup send-events timer (100 ms)
		m_TimerSendEvents.setTimeoutFunction([this]() { Handle_TimerSendEvents(); });
		std::chrono::milliseconds defaultElapsedPeriod(100);
		m_TimerSendEvents.setElapsedPeriod(defaultElapsedPeriod);

		// Setup physics tick timer (50 ms = 20 Hz)
		m_LastPhysicsTick = std::chrono::steady_clock::now();
		m_TimerPhysicsTick.setTimeoutFunction([this]() { Handle_TimerPhysicsTick(); });
		m_TimerPhysicsTick.setElapsedPeriod(std::chrono::milliseconds(50));
	}

	void Server::LoadConfiguration()
	{
		m_Config.Load(m_ConfigFilePath);

		// Apply configuration to the network server
		m_NetworkServer.SetServerPort(m_Config.serverData.Port);
	}

	void Server::SaveConfiguration()
	{
		m_Config.Save(m_ConfigFilePath);
	}

	void Server::UpdateMOTD()
	{
		ServerMotdMsg motdMsg;
		motdMsg.ServerMotd = m_Config.serverData.MOTD;
		motdMsg.MaxPlayers = 20;

		auto players = m_WorldManager->GetAllPlayers();
		for (auto& [clientHandle, playerInfo] : players)
		{
			motdMsg.PlayerNames.push_back(playerInfo->GetName());
		}

		motdMsg.PlayerCount = static_cast<int>(motdMsg.PlayerNames.size());

		// Load Icon png data
		std::filesystem::path iconPath = Utils::GetExecutableDirectory() / "ServerThumbnail.png";
		if (std::filesystem::exists(iconPath))
		{
			std::ifstream file(iconPath, std::ios::binary);
			if (!file.is_open())
			{
				std::cerr << "Failed to open icon file for reading: " << iconPath << "\n";
				throw std::runtime_error("Failed to open icon file for reading: " + iconPath.string());
			}
			motdMsg.ServerIconPngData = std::vector<uint8_t>(std::istreambuf_iterator<char>(file), {});
		}
		m_NetworkServer.SetMOTD(motdMsg);
	}

	void Server::SubscribeToNetworkServerEvents()
	{
		m_NetworkServerEventHandles.push_back(m_NetworkServer.EvtClientConnected.Subscribe(
			[this](const NetworkServer::ClientConnectedEventArgs& args) { Handle_ClientConnected(args); }));

		m_NetworkServerEventHandles.push_back(m_NetworkServer.EvtClientDisconnected.Subscribe(
			[this](const NetworkServer::ClientDisconnectedEventArgs& args) { Handle_ClientDisconnected(args); }));

		m_NetworkServerEventHandles.push_back(m_NetworkServer.EvtMessageReceived.Subscribe(
			[this](const NetworkServer::MessageReceivedEventArgs& args) { Handle_NetworkMessageReceived(args); }));
	}

	void Server::Handle_NetworkMessageReceived(const NetworkServer::MessageReceivedEventArgs& args)
	{
		std::visit(
			[&](const auto& msg)
			{
				using T = std::decay_t<decltype(msg)>;
				if constexpr (std::is_same_v<T, ClientInfoMsg>)
				{
					Handle_ClientInfoMsgReceived(args, msg);
				}
				else if constexpr (std::is_same_v<T, PlayerInfoMsg>)
				{
					Handle_PlayerInfoMsgReceived(args, msg);
				}
				else if constexpr (std::is_same_v<T, RequestChunksMsg>)
				{
					Handle_PlayerRequestChunksMsgReceived(args, msg);
				}
				else if constexpr (std::is_same_v<T, BlocksChangedMsg>)
				{
					Handle_BlocksChangedMsgReceived(args, msg);
				}
				else if constexpr (std::is_same_v<T, ItemDroppedMsg>)
				{
					Handle_ItemDroppedMsgReceived(args, msg);
				}
				else
				{
					std::cout << "Received unhandled message type from client " << args.Sender << "\n";
				}
			},
			args.Message);
	}

	void Server::Handle_ClientInfoMsgReceived(const NetworkServer::MessageReceivedEventArgs& args,
											  const ClientInfoMsg& msg)
	{
		std::cout << "Received ClientInfoMsg from client " << args.Sender << ": PlayerName=" << msg.PlayerName
				  << ", UUID=" << msg.UUID << "\n";

		// Update player info
		PlayerInfo playerInfo;
		playerInfo.ClientHandle = args.Sender;
		playerInfo.PlayerName = msg.PlayerName;
		playerInfo.UUID = msg.UUID;

		//AddOrUpdatePlayer(playerInfo);
	}

	void Server::Handle_PlayerInfoMsgReceived(const NetworkServer::MessageReceivedEventArgs& args,
											  const PlayerInfoMsg& msg)
	{
		(void) args; // Unused parameter

		//std::cout << "Received PlayerInfoMsg from client " << args.Sender << ": Username=" << msg.Username
		//		  << ", UUID=" << msg.UUID << ", Position=" << msg.Position.x << "," << msg.Position.y << ","
		//		  << msg.Position.z << "\n";

		// Update player
		std::shared_ptr<Player> deserializedPlayer = SerializerDTO::DeserializePlayer(msg.player);

		m_WorldManager->UpdatePlayer(deserializedPlayer);
	}

	void Server::Handle_TimerSendEvents()
	{
		// Sends Entity Snapshot to all clients
		std::unordered_map<std::string, std::shared_ptr<Player>> players = m_WorldManager->GetAllPlayers();

		if (players.empty())
		{
			return; // No players connected, skip sending snapshot
		}

		EntitySnapshotMsg entitySnapshotMsg;
		for (const auto& [clientHandle, player] : players)
		{
			PlayerDTO playerDTO = SerializerDTO::SerializePlayer(*player);
			entitySnapshotMsg.Players.push_back(std::move(playerDTO));
		}

		auto entities = m_WorldManager->GetAllEntities();
		for (const auto& entity : entities)
		{
			EntityDTO entityDTO = SerializerDTO::SerializeEntity(*entity);
			entitySnapshotMsg.Entities.push_back(std::move(entityDTO));
		}

		m_NetworkServer.Broadcast(entitySnapshotMsg);
	}

	void Server::Handle_PlayerRequestChunksMsgReceived(const NetworkServer::MessageReceivedEventArgs& args,
													   const RequestChunksMsg& msg)
	{
		std::cout << "Received RequestChunksMsg from client " << args.Sender << ": Requested "
				  << msg.requestedChunks.size() << " chunks\n";

		for (const auto& chunkPos : msg.requestedChunks)
		{
			auto chunk = m_WorldManager->GetChunk(chunkPos);
			if (chunk)
			{
				ChunkDTO chunkDto = SerializerDTO::SerializeChunk(chunk);
				ChunkDataMsg chunkDataMsg;
				chunkDataMsg.Chunk = std::move(chunkDto);
				//std::cout << "(" << chunkPos.x << ", " << chunkPos.y << ")" << std::endl;
				m_NetworkServer.Send(args.Sender, chunkDataMsg);
			}
		}
	}

	void Server::Handle_BlocksChangedMsgReceived(const NetworkServer::MessageReceivedEventArgs& args,
												 const BlocksChangedMsg& msg)
	{
		// Retreve ClientId
		uint32_t clientHandle = args.Sender;

		std::cout << "Received BlocksChangedMsg from client " << clientHandle << ": " << msg.ChangedBlocks.size()
				  << " changed blocks\n";

		std::vector<Block> changedBlocks;
		for (const auto& blockDTO : msg.ChangedBlocks)
		{
			changedBlocks.emplace_back(SerializerDTO::DeserializeBlock(blockDTO));
		}

		m_WorldManager->SetBlocks(changedBlocks, WorldManager::BlocksChangedEventArgs::eOrigin::ClientRequest, true);
	}

	void Server::Handle_ItemDroppedMsgReceived(const NetworkServer::MessageReceivedEventArgs& args,
											   const ItemDroppedMsg& msg)
	{
		// Resolve sender's player
		std::string playerUUID;
		{
			std::shared_lock lock(m_MutexPlayers);
			auto it = m_ClientHandleToPlayerInfo.find(args.Sender);
			if (it == m_ClientHandleToPlayerInfo.end())
				return;
			playerUUID = it->second.UUID;
		}

		std::shared_ptr<Player> player = m_WorldManager->GetPlayer(playerUUID);
		if (!player)
			return;

		// Spawn position: player head (eye position)
		const glm::vec3 spawnPos = player->GetEyePosition();

		// Initial velocity: forward direction * 7 m/s + small upward nudge
		const glm::vec3 forward = player->GetFacing();
		const glm::vec3 initialVelocity = forward * 7.f + glm::vec3(0.f, 2.5f, 0.f);

		// Create the BlockEntity
		auto blockEntity = std::make_shared<BlockEntity>(Utils::GenerateUUID());
		blockEntity->SetSlot(Slot{static_cast<BlockId>(msg.BlockId), msg.Count});

		Transform t = blockEntity->GetTransform();
		t.Position = spawnPos;
		blockEntity->SetTransform(t);

		PhysicsBody pb = blockEntity->GetPhysicsBody();
		pb.Velocity = initialVelocity;
		blockEntity->SetPhysicsBody(pb);

		m_WorldManager->AddEntity(blockEntity);

		std::cout << "Spawned BlockEntity UUID=" << blockEntity->UUID << " for player " << playerUUID << "\n";
	}

	void Server::Handle_TimerPhysicsTick()
	{
		// Compute delta time
		auto now = std::chrono::steady_clock::now();
		float deltaTime = std::chrono::duration<float>(now - m_LastPhysicsTick).count();
		m_LastPhysicsTick = now;

		// Update physics for all entities (includes BlockEntities)
		m_PhysicsEngine->Update(deltaTime);

		// Tick cooldowns/lifetimes and collect BlockEntities
		auto entities = m_WorldManager->GetAllEntities();
		std::vector<std::shared_ptr<BlockEntity>> blockEntities;
		std::vector<std::string> toRemove;

		for (const auto& entity : entities)
		{
			if (entity->Type != EntityType::Block)
				continue;

			auto blockEntity = std::static_pointer_cast<BlockEntity>(entity);
			blockEntity->DecrementPickupCooldown(deltaTime);
			blockEntity->DecrementLifetime(deltaTime);

			if (blockEntity->IsExpired())
				toRemove.push_back(entity->UUID);
			else
				blockEntities.push_back(blockEntity);
		}

		for (const auto& uuid : toRemove)
		{
			m_WorldManager->RemoveEntity(uuid);
			std::cout << "Despawned expired BlockEntity UUID=" << uuid << "\n";
		}

		// -----------------------------------------------------------------
		// Pickup detection: test AABB overlap between each player and each
		// BlockEntity that has completed its pickup cooldown.
		// -----------------------------------------------------------------
		auto players = m_WorldManager->GetAllPlayers();

		for (auto& [playerUUID, player] : players)
		{
			if (!player->HasTransform() || !player->HasPhysicsBody())
				continue;

			// Find the player's ClientHandle for sending messages
			auto infoIt = m_UUIDToPlayerInfo.find(playerUUID);
			if (infoIt == m_UUIDToPlayerInfo.end())
				continue;
			const uint32_t clientHandle = infoIt->second.ClientHandle;

			// Player AABB
			const glm::vec3 playerPos = player->GetTransform().Position;
			const PhysicsBody playerPb = player->GetPhysicsBody();
			const glm::vec3 playerCenter = playerPos + playerPb.CenterOffset;
			const glm::vec3 playerHalf = playerPb.Size * 0.5f;
			const glm::vec3 playerMin = playerCenter - playerHalf;
			const glm::vec3 playerMax = playerCenter + playerHalf;

			for (auto& blockEntity : blockEntities)
			{
				if (!blockEntity->CanBePickedUp())
					continue;
				if (!blockEntity->HasTransform() || !blockEntity->HasPhysicsBody())
					continue;

				// BlockEntity AABB
				const glm::vec3 bePos = blockEntity->GetTransform().Position;
				const PhysicsBody bePb = blockEntity->GetPhysicsBody();
				const glm::vec3 beCenter = bePos + bePb.CenterOffset;
				const glm::vec3 beHalf = bePb.Size * 0.5f;
				const glm::vec3 beMin = beCenter - beHalf;
				const glm::vec3 beMax = beCenter + beHalf;

				// AABB overlap test
				const bool overlaps = playerMax.x > beMin.x && playerMin.x < beMax.x &&
									  playerMax.y > beMin.y && playerMin.y < beMax.y &&
									  playerMax.z > beMin.z && playerMin.z < beMax.z;
				if (!overlaps)
					continue;

				// ---------------------------------------------------------
				// Pickup logic: partial pickup supported.
				// Pass 1 — fill existing matching stacks (hotbar first).
				// Pass 2 — fill empty slots (hotbar first).
				// ---------------------------------------------------------
				const BlockId blockId = blockEntity->GetSlot().Id;
				int remaining = static_cast<int>(blockEntity->GetSlot().Count);

				if (remaining <= 0 || blockId == BlockId::Air)
					continue;

				// We need mutable inventory copies — modify, then write back.
				Inventory hotbar = player->GetHotbar();
				Inventory inventory = player->GetPlayerInventory();

				// tryAddInv: iterate hotbar then main inventory, either filling
				// existing matching stacks (mustMatch=true) or empty slots (mustMatch=false).
				auto tryAddInv = [&](bool mustMatch)
				{
					for (int i = 0; i < hotbar.Rows() * hotbar.Columns() && remaining > 0; ++i)
					{
						Slot& slot = hotbar.At(i);
						if (mustMatch)
						{
							if (slot.Id != blockId || slot.Count >= k_MaxStackSize)
								continue;
						}
						else
						{
							if (!slot.IsEmpty())
								continue;
						}
						const int canAdd = mustMatch
							? std::min(static_cast<int>(k_MaxStackSize - slot.Count), remaining)
							: std::min(static_cast<int>(k_MaxStackSize), remaining);
						if (canAdd <= 0)
							continue;
						if (mustMatch)
							slot.Count += static_cast<uint8_t>(canAdd);
						else
							slot = Slot{blockId, static_cast<uint8_t>(canAdd)};
						remaining -= canAdd;

						ItemPickedUpMsg msg;
						msg.IsHotbar = true;
						msg.Index = static_cast<uint8_t>(i);
						msg.ItemId = static_cast<uint16_t>(blockId);
						msg.Count = slot.Count;
						m_NetworkServer.Send(clientHandle, msg);
					}

					for (int i = 0; i < inventory.Rows() * inventory.Columns() && remaining > 0; ++i)
					{
						Slot& slot = inventory.At(i);
						if (mustMatch)
						{
							if (slot.Id != blockId || slot.Count >= k_MaxStackSize)
								continue;
						}
						else
						{
							if (!slot.IsEmpty())
								continue;
						}
						const int canAdd = mustMatch
							? std::min(static_cast<int>(k_MaxStackSize - slot.Count), remaining)
							: std::min(static_cast<int>(k_MaxStackSize), remaining);
						if (canAdd <= 0)
							continue;
						if (mustMatch)
							slot.Count += static_cast<uint8_t>(canAdd);
						else
							slot = Slot{blockId, static_cast<uint8_t>(canAdd)};
						remaining -= canAdd;

						ItemPickedUpMsg msg;
						msg.IsHotbar = false;
						msg.Index = static_cast<uint8_t>(i);
						msg.ItemId = static_cast<uint16_t>(blockId);
						msg.Count = slot.Count;
						m_NetworkServer.Send(clientHandle, msg);
					}
				};

				const int originalRemaining = remaining;
				tryAddInv(true);  // Pass 1: fill existing stacks
				tryAddInv(false); // Pass 2: fill empty slots

				// Write inventories back if anything was picked up
				if (remaining < originalRemaining)
				{
					player->SetHotbar(hotbar);
					player->SetPlayerInventory(inventory);
				}

				if (remaining <= 0)
				{
					// Fully picked up — remove entity
					toRemove.push_back(blockEntity->UUID);
				}
				else
				{
					// Partial pickup — update remaining count and reset lifetime
					blockEntity->SetSlot(Slot{blockId, static_cast<uint8_t>(remaining)});
					blockEntity->SetLifetime(BlockEntity::DefaultLifetime);
				}
			}

			// Remove any fully-picked-up entities (avoid double-remove with expired list)
			for (const auto& uuid : toRemove)
			{
				m_WorldManager->RemoveEntity(uuid);
			}
			toRemove.clear();
		}
	}

	void Server::Handle_ClientConnected(const NetworkServer::ClientConnectedEventArgs& args)
	{
		std::cout << "New client connected: " << args.Client << " (" << args.PlayerName << ", " << args.UUID << ", "
				  << args.IpAddress << ")\n";

		PlayerInfo playerInfo;
		playerInfo.ClientHandle = args.Client;
		playerInfo.PlayerName = args.PlayerName;
		playerInfo.UUID = args.UUID;

		// Add the new Player
		AddPlayer(playerInfo);
		m_WorldManager->RequestAllMissingChunks();

		ServerInfoMsg srvInfoMsg;
		srvInfoMsg.ServerName = m_Config.serverData.ServerName;
		srvInfoMsg.ClientHandle = args.Client;
		srvInfoMsg.SimulationDistance = m_Config.serverData.SimulationDistance;

		// Sends the ServerInfoMsg to the newly connected client
		m_NetworkServer.Send(args.Client, srvInfoMsg);

		// Sends the Entity snapshot to all clients
		Handle_TimerSendEvents();

		UpdateMOTD();
	}

	void Server::Handle_ClientDisconnected(const NetworkServer::ClientDisconnectedEventArgs& args)
	{
		std::cout << "Client disconnected: " << args.Client << " ( " << args.UUID << ", " << args.IpAddress << ")\n";

		RemovePlayer(args.UUID);

		UpdateMOTD();
	}

	void Server::Handle_ChunkAdded(const std::shared_ptr<Chunk>& chunk)
	{
		// Send the chunk to all nearby players.
		uint8_t chunkLoadingDistance = m_Config.serverData.SimulationDistance;
		glm::ivec2 chunkPosition = chunk->GetPosition();
		std::unordered_map<std::string, glm::vec3> playersPosition = m_WorldManager->GetPlayersPosition();

		std::vector<std::string> nearbyPlayers;
		for (const auto& [playerUUID, playerPosition] : playersPosition)
		{
			glm::ivec2 playerChunkPos = Utils::WorldToChunkPosition(playerPosition);
			if (std::abs(playerChunkPos.x - chunkPosition.x) <= chunkLoadingDistance &&
				std::abs(playerChunkPos.y - chunkPosition.y) <= chunkLoadingDistance)
			{
				nearbyPlayers.push_back(playerUUID);
			}
		}

		if (!nearbyPlayers.empty())
		{
			ChunkDTO chunkDto = SerializerDTO::SerializeChunk(chunk);
			ChunkDataMsg chunkDataMsg;
			chunkDataMsg.Chunk = std::move(chunkDto);

			//std::cout << "Sending chunk at position: (" << chunkPosition.x << ", " << chunkPosition.y << ") to "
			//		  << nearbyPlayers.size() << " nearby players\n";

			for (const auto& playerUUID : nearbyPlayers)
			{
				uint32_t clientHandle;
				{
					std::shared_lock lock(m_MutexPlayers);
					auto it = m_UUIDToPlayerInfo.find(playerUUID);
					if (it != m_UUIDToPlayerInfo.end())
					{
						clientHandle = it->second.ClientHandle;
					}
					else
					{
						continue; // Player not found, skip sending chunk
					}
				}
				m_NetworkServer.Send(clientHandle, chunkDataMsg);
			}
		}
	}

	void Server::Handle_ChunkRemoved(const std::shared_ptr<Chunk>& chunk)
	{
		// For now, the client has the resposiblity to remove it's chunks.
		(void) chunk; // Unused parameter
	}

	void Server::Handle_BlocksChanged(const WorldManager::BlocksChangedEventArgs& args)
	{
		BlocksChangedMsg blocksChangedMsg;
		for (const auto& block : args.ChangedBlocks)
		{
			blocksChangedMsg.ChangedBlocks.emplace_back(SerializerDTO::SerializeBlock(block));
		}

		m_NetworkServer.Broadcast(blocksChangedMsg);
	}

	void Server::AddPlayer(const PlayerInfo& playerInfo)
	{
		std::shared_ptr<Player> loadedPlayer = m_WorldManager->LoadPlayer(playerInfo.UUID);

		// If the player already exists in the world manager, update the name in case it has changed since last connection
		if (loadedPlayer)
		{
			loadedPlayer->SetName(playerInfo.PlayerName);
		}
		else // Create a new player and add it to the world manager
		{
			std::shared_ptr<Player> player = std::make_shared<Player>(playerInfo.UUID);
			player->SetName(playerInfo.PlayerName);
			glm::vec3 spawnPosition = m_WorldManager->GetSpawnPosition();
			player->SetPosition(spawnPosition);

			m_WorldManager->AddPlayer(player);
		}

		{
			std::lock_guard lock(m_MutexPlayers);
			m_ClientHandleToPlayerInfo[playerInfo.ClientHandle] = playerInfo;
			m_UUIDToPlayerInfo[playerInfo.UUID] = playerInfo;
		}
	}

	void Server::RemovePlayer(const std::string& uuid)
	{
		m_WorldManager->RemovePlayer(uuid);
		{
			std::lock_guard lock(m_MutexPlayers);
			auto it = m_UUIDToPlayerInfo.find(uuid);
			if (it != m_UUIDToPlayerInfo.end())
			{
				uint32_t clientHandle = it->second.ClientHandle;
				m_ClientHandleToPlayerInfo.erase(clientHandle);
				m_UUIDToPlayerInfo.erase(it);
			}
		}
	}

	void Server::SubscribeToWorldManagerEvents()
	{
		m_WorldManagerEventHandles.push_back(m_WorldManager->EvtChunkAdded.Subscribe(
			[this](const std::shared_ptr<Chunk>& chunk) { Handle_ChunkAdded(chunk); }));

		m_WorldManagerEventHandles.push_back(m_WorldManager->EvtChunkRemoved.Subscribe(
			[this](const std::shared_ptr<Chunk>& chunk) { Handle_ChunkRemoved(chunk); }));

		m_WorldManagerEventHandles.push_back(m_WorldManager->EvtBlocksChanged.Subscribe(
			[this](const WorldManager::BlocksChangedEventArgs& args) { Handle_BlocksChanged(args); }));
	}

} // namespace onion::voxel
