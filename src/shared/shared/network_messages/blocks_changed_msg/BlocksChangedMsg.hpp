#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/vector.hpp>

#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <shared/network_messages/DTOs/DTOs.hpp>
#include <shared/network_messages/GlmSerialization.hpp>
#include <shared/network_messages/MessageHeader.hpp>

namespace onion::voxel
{
	struct BlocksChangedMsg
	{
		static constexpr MessageHeader::eType StaticType = MessageHeader::eType::BlocksChanged;

		// List of changed blocks with their world positions
		std::vector<BlockDTO> ChangedBlocks;

		template <class Archive> void serialize(Archive& ar) { ar(ChangedBlocks); }
	};
} // namespace onion::voxel
