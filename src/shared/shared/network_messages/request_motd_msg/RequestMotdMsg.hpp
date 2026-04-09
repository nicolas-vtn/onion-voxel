#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>

#include <sstream>
#include <string>

#include <shared/network_messages/MessageHeader.hpp>

namespace onion::voxel
{
	struct RequestMotdMsg
	{
		static constexpr MessageHeader::eType StaticType = MessageHeader::eType::RequestMotd;

		template <class Archive> void serialize(Archive& ar) { (void) ar; }
	};
} // namespace onion::voxel
