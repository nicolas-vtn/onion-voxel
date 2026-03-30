#pragma once

#include <memory>

#include <shared/world/chunk/Chunk.hpp>

#include <shared/world/world_save/SaveFormats/ChunkSave.hpp>

namespace onion::voxel
{
	class SerializerSave
	{
		// ----- CHUNK -----
	  public:
		static std::vector<uint8_t> SerializeChunk(const std::shared_ptr<Chunk>& chunk);
		static std::shared_ptr<Chunk> DeserializeChunk(const std::vector<uint8_t>& data);

		// ----- CHUNK SAVE -----
	  private:
		static ChunkSave SerializeChunkToSave(const std::shared_ptr<Chunk>& chunk);
		static std::shared_ptr<Chunk> DeserializeChunk(const ChunkSave& dto);

		// ----- SUB CHUNK -----
	  private:
		static SubChunkSave SerializeSubChunk(const SubChunk& sc);
		static SubChunk DeserializeSubChunk(const SubChunkSave& dto);

		// ----- BLOCK STATE -----
	  private:
		static BlockStateSave SerializeBlockState(const BlockState& block);
		static BlockState DeserializeBlockState(const BlockStateSave& dto);
	};
} // namespace onion::voxel
