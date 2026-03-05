#pragma once

#include <glm/glm.hpp>

#include <shared_mutex>
#include <vector>

#include "../block/Block.hpp"
#include "SubChunk.hpp"

namespace onion::voxel
{
	class Chunk
	{
		// ----- Constructor / Destructor -----
	  public:
		Chunk(const glm::ivec2& position);
		~Chunk();

		// ----- Public API -----
	  public:
		// ----- Getters / Setters -----
		glm::ivec2 GetPosition() const;

		Block GetBlock(const glm::ivec3& localPosition) const;
		void SetBlock(const glm::ivec3& localPosition, const Block& block);

		int GetSubChunkCount() const;

		// ----- Members -----
	  private:
		const glm::ivec2 m_Position;	   // Position of the chunk in chunk coordinates (Not in world coordinates)
		mutable std::shared_mutex m_Mutex; // Mutex for synchronizing access to the chunk data
		std::vector<SubChunk> m_SubChunks; // The subchunks that make up this chunk
		std::vector<Block> m_BlocksPalette{Block()}; // The blocks palette that make up this chunk

		// ----- Private Methods -----
	  private:
		/// @brief Adds a block to the blocks palette if it doesn't already exist and returns the block data index for the block
		/// @param block The block to add to the palette
		/// @return The block data index for the block in the palette
		uint8_t AddBlockToPalette(const Block& block);
	};
} // namespace onion::voxel
