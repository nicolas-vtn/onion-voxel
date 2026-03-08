#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>

#include <sstream>
#include <string>

#include "../MessageHeader.hpp"

namespace onion::voxel
{
	struct ServerInfoMsg
	{
		static constexpr MessageHeader::eType StaticType = MessageHeader::eType::ServerInfo;

		std::string ServerName;
		uint32_t ClientHandle;

		template <class Archive> void serialize(Archive& ar) { ar(ServerName, ClientHandle); }
	};
} // namespace onion::voxel
