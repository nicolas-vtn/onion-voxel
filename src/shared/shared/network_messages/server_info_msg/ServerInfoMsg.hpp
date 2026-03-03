#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>

#include <sstream>
#include <string>

namespace onion::voxel
{
	struct ServerInfoMsg
	{
		static constexpr MessageHeader::eType StaticType = MessageHeader::eType::ServerInfo;

		std::string Msg;

		template <class Archive> void serialize(Archive& ar) { ar(Msg); }
	};
} // namespace onion::voxel
