#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>

namespace onion::voxel
{
	struct SubChunkDTO
	{
		bool isMono;
		uint8_t monoIndex;
		std::vector<uint8_t> indices;

		template <class Archive> void serialize(Archive& ar) { ar(isMono, monoIndex, indices); }
	};
} // namespace onion::voxel
