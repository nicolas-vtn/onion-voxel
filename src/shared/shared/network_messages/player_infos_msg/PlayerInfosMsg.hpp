#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>

#include <sstream>
#include <string>

#include "../MessageHeader.hpp"

#include "../GlmSerialization.hpp"

namespace onion::voxel
{
	struct PlayerInfoMsg
	{
		static constexpr MessageHeader::eType StaticType = MessageHeader::eType::PlayerInfos;

		std::string Username;
		std::string UUID;

		glm::vec3 Position;

		template <class Archive> void serialize(Archive& ar) { ar(Username, UUID, Position); }
	};
} // namespace onion::voxel
