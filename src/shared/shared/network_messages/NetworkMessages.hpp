#pragma once

#include "MessageHeader.hpp"

#include "chunk_data_msg/ChunkDataMsg.hpp"
#include "client_info_msg/ClientInfoMsg.hpp"
#include "player_infos_msg/PlayerInfosMsg.hpp"
#include "server_info_msg/ServerInfoMsg.hpp"

#include "DTOs/BlockDTO.hpp"
#include "DTOs/SubChunkDTO.hpp"

#include <variant>

namespace onion::voxel
{
	using NetworkMessage = std::variant<ClientInfoMsg, ServerInfoMsg, ChunkDataMsg, PlayerInfoMsg>;

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

			default:
				throw std::runtime_error("Unknown message type");
		}
	}
} // namespace onion::voxel
