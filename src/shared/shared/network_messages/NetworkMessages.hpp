#pragma once

#include "MessageHeader.hpp"

#include "client_info_msg/ClientInfoMsg.hpp"
#include "server_info_msg/ServerInfoMsg.hpp"

#include <variant>

namespace onion::voxel
{
	using NetworkMessage = std::variant<ClientInfoMsg, ServerInfoMsg>;

	inline NetworkMessage DeserializeMessage(cereal::BinaryInputArchive& archive, MessageHeader::eType type)
	{
		switch (type)
		{
			case MessageHeader::eType::ClientInfo:
				{
					ClientInfoMsg msg;
					archive(msg);

					std::cout << "Deserialized ClientInfoMsg: Username=" << msg.Username << ", UUID=" << msg.UUID
							  << std::endl;

					return msg;
				}

			case MessageHeader::eType::ServerInfo:
				{
					ServerInfoMsg msg;
					archive(msg);

					std::cout << "Deserialized ServerInfoMsg: ServerName=" << msg.ServerName << std::endl;

					return msg;
				}

			default:
				throw std::runtime_error("Unknown message type");
		}
	}
} // namespace onion::voxel
