#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>

#include <sstream>
#include <string>

#include <shared/network_messages/GlmSerialization.hpp>
#include <shared/network_messages/MessageHeader.hpp>

namespace onion::voxel
{
	struct PlayerInfoMsg
	{
		static constexpr MessageHeader::eType StaticType = MessageHeader::eType::PlayerInfos;

		std::string Username;
		std::string UUID;

		glm::vec3 Position{};
		glm::vec3 Facing{};

		template <class Archive> void serialize(Archive& ar) { ar(Username, UUID, Position, Facing); }
	};
} // namespace onion::voxel
