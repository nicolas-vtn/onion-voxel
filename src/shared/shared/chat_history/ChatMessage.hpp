#pragma once

#include <string>

#include <onion/DateTime.hpp>

namespace onion::voxel
{
	struct ChatMessage
	{
		const DateTime Timestamp;
		const std::string PlayerName;
		const std::string PlayerUUID;
		const std::string Content;
	};
} // namespace onion::voxel
