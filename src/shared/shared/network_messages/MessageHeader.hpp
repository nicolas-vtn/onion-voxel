#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>

#include <cstdint>
#include <string>

namespace onion::voxel
{
	struct MessageHeader
	{
		enum class eType : uint8_t
		{
			None = 0,
			ServerInfo,
			ClientInfo,
		};

		eType Type = eType::None;
		std::string SenderId;

		template <class Archive> void serialize(Archive& archive)
		{
			uint8_t type = static_cast<uint8_t>(Type);
			archive(type, SenderId);

			if constexpr (Archive::is_loading::value)
				Type = static_cast<eType>(type);
		}
	};
} // namespace onion::voxel
