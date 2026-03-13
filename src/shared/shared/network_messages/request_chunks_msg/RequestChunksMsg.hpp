#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>

#include <sstream>
#include <string>

#include <shared/network_messages/GlmSerialization.hpp>
#include <shared/network_messages/MessageHeader.hpp>

namespace onion::voxel
{
	struct RequestChunksMsg
	{
		static constexpr MessageHeader::eType StaticType = MessageHeader::eType::RequestChunks;

		std::vector<glm::ivec2> requestedChunks;

		template <class Archive> void serialize(Archive& ar) { ar(requestedChunks); }
	};
} // namespace onion::voxel
