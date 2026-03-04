#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>

#include <sstream>
#include <string>

namespace onion::voxel
{
	struct ClientInfoMsg
	{
		static constexpr MessageHeader::eType StaticType = MessageHeader::eType::ClientInfo;

		std::string Username;
		std::string UUID;

		template <class Archive> void serialize(Archive& ar) { ar(Username, UUID); }
	};
} // namespace onion::voxel
