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

		/// @brief Pre-register a block in the palette and return its index. Useful to batch-resolve
		/// palette indices before tight fill loops, avoiding repeated linear scans.
		uint16_t GetOrAddPaletteIndex(const BlockState& block);

		/// @brief Fill a world-local y-range [yMin, yMax] in a single (x, z) column with a pre-resolved
		/// palette index. Handles spanning multiple SubChunks. No locking — caller must ensure thread safety.
		void FillColumn_Unsafe(uint8_t x, uint16_t yMin, uint16_t yMax, uint8_t z, uint16_t paletteIndex);

		int GetSubChunkCount() const;
		int GetChunkHeight() const;
		int GetHeightAt(int x, int z) const; // Get the height at (x, z) in local , or -1

		bool IsSubchunkMonoBlock(const int subChunkIndex) const;

		// ----- Members -----
	  protected:
		const glm::ivec2 m_Position;	   // Position of the chunk in chunk coordinates (Not in world coordinates)
		mutable std::shared_mutex m_Mutex; // Mutex for synchronizing access to the chunk data
		std::vector<SubChunk> m_SubChunks; // The subchunks that make up this chunk
		std::vector<BlockState> m_BlocksPalette{BlockState(BlockId::Air)}; // The blocks palette that make up this chunk
		int m_ChunkHeight = 0;											   // The height of the chunk in blocks
	};
} // namespace onion::voxel
