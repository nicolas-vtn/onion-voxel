#pragma once

#include <variant>

#include "MessageHeader.hpp"

#include "blocks_changed_msg/BlocksChangedMsg.hpp"
#include "chunk_data_msg/ChunkDataMsg.hpp"
#include "client_info_msg/ClientInfoMsg.hpp"
#include "entity_snapshot_msg/EntitySnapshotMsg.hpp"
#include "player_infos_msg/PlayerInfosMsg.hpp"
#include "request_chunks_msg/RequestChunksMsg.hpp"
#include "server_info_msg/ServerInfoMsg.hpp"

#include <shared/data_transfer_objects/DTOs/DTOs.hpp>

namespace onion::voxel
{
	using NetworkMessage = std::variant<ClientInfoMsg,
										ServerInfoMsg,
										ChunkDataMsg,
										PlayerInfoMsg,
										BlocksChangedMsg,
										RequestChunksMsg,
										EntitySnapshotMsg>;

	inline NetworkMessage DeserializeMessage(cereal::BinaryInputArchive& archive, MessageHeader::eType type)
	{
		switch (type)
		{
			case MessageHeader::eType::ClientInfo:
				{
					ClientInfoMsg msg;
					archive(msg);
					return msg;
				}

			case MessageHeader::eType::ServerInfo:
				{
					ServerInfoMsg msg;
					archive(msg);
					return msg;
				}

			case MessageHeader::eType::ChunkData:
				{
					ChunkDataMsg msg;
					archive(msg);
					return msg;
				}

			case MessageHeader::eType::PlayerInfos:
				{
					PlayerInfoMsg msg;
					archive(msg);
					return msg;
				}

			case MessageHeader::eType::BlocksChanged:
				{
					BlocksChangedMsg msg;
					archive(msg);
					return msg;
				}

			case MessageHeader::eType::RequestChunks:
				{
					RequestChunksMsg msg;
					archive(msg);
					return msg;
				}

			case MessageHeader::eType::EntitySnapshot:
				{
					EntitySnapshotMsg msg;
					archive(msg);
					return msg;
				}

			default:
				throw std::runtime_error("Unknown message type");
		}
	}
} // namespace onion::voxel
