#pragma once

#include <string>

#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>

#include <shared/network_messages/MessageHeader.hpp>

namespace onion::voxel
{
	// Sent by the client when the player sends a chat message.
	// PlayerName and UUID are empty when sent client → server (the server resolves them from its own registry).
	// They are populated by the server when broadcasting the message to all clients.
	struct ChatMsg
	{
		static constexpr MessageHeader::eType StaticType = MessageHeader::eType::Chat;

		std::string PlayerName;
		std::string PlayerUUID;
		std::string Message;

		template <class Archive> void serialize(Archive& ar) { ar(PlayerName, PlayerUUID, Message); }
	};
} // namespace onion::voxel
