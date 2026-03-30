#pragma once

#include <cereal/archives/binary.hpp>

#include <sstream>
#include <string>

#include <shared/network_messages/GlmSerialization.hpp>

#include "BlockStateSave.hpp"
#include "SubChunkSave.hpp"

namespace onion::voxel
{
	struct ChunkSave
	{
		glm::ivec2 Position{};

		std::vector<SubChunkSave> SubChunks;
		std::vector<BlockStateSave> Palette;

		template <class Archive> void serialize(Archive& ar) { ar(Position, SubChunks, Palette); }
	};
} // namespace onion::voxel
