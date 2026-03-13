#pragma once

#include <memory>

#include "NetworkMessages.hpp"

#include <shared/world/chunk/Chunk.hpp>

namespace onion::voxel
{
	class Serializer
	{
		// ----- CHUNK -----
	  public:
		static ChunkDataMsg SerializeChunk(std::shared_ptr<Chunk> chunk);
		static std::shared_ptr<Chunk> DeserializeChunk(const ChunkDataMsg& msg);

		// ----- SUB CHUNK -----
	  public:
		static SubChunkDTO SerializeSubChunk(const SubChunk& sc);
		static SubChunk DeserializeSubChunk(const SubChunkDTO& dto);

		// ----- BLOCK STATE -----
	  public:
		static BlockStateDTO SerializeBlockState(const BlockState& block);
		static BlockState DeserializeBlockState(const BlockStateDTO& dto);

		// ----- BLOCK -----
	  public:
		static BlockDTO SerializeBlock(const Block& block);
		static Block DeserializeBlock(const BlockDTO& dto);
	};
} // namespace onion::voxel
