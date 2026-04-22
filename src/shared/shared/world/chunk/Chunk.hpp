#pragma once

#include <glm/glm.hpp>

#include <mutex>
#include <shared_mutex>
#include <vector>

#include <shared/world/block/Block.hpp>

#include "SubChunk.hpp"

namespace onion::voxel
{
	class SerializerDTO;
	class SerializerSave;

	class Chunk
	{
		friend class SerializerDTO;
		friend class SerializerSave;

		// ----- Constructor / Destructor -----
	  public:
		Chunk(const glm::ivec2& position, const size_t subChunkCount = 0);
		Chunk(glm::ivec2 position, std::vector<SubChunk> subChunks, std::vector<BlockState> blocksPalette);
		~Chunk();

		// ----- Public API -----
	  public:
		void Optimize(); // Optimize the chunk data (e.g., optimize subchunks, remove unused blocks from palette)

		// ----- Getters / Setters -----
		glm::ivec2 GetPosition() const;

		BlockState GetBlock(const glm::ivec3& localPosition) const;
		void SetBlock(const glm::ivec3& localPosition, const BlockState& block);

		void SetBlock_Unsafe(const uint8_t x, const uint16_t y, const uint8_t z, const BlockState& block);

		int GetSubChunkCount() const;

		bool IsSubchunkMonoBlock(const int subChunkIndex) const;

		// ----- Members -----
	  protected:
		const glm::ivec2 m_Position;	   // Position of the chunk in chunk coordinates (Not in world coordinates)
		mutable std::shared_mutex m_Mutex; // Mutex for synchronizing access to the chunk data
		std::vector<SubChunk> m_SubChunks; // The subchunks that make up this chunk
		std::vector<BlockState> m_BlocksPalette{BlockState(BlockId::Air)}; // The blocks palette that make up this chunk

		// ----- Private Methods -----
	  private:
		/// @brief Adds a block to the blocks palette if it doesn't already exist and returns the block data index for the block
		/// @param block The block to add to the palette
		/// @return The block data index for the block in the palette
		uint8_t AddBlockToPalette(const BlockState& block);
	};
} // namespace onion::voxel
