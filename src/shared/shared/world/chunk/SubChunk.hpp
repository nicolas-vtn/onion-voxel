#pragma once

#include <glm/glm.hpp>

#include <array>
#include <memory>
#include <vector>

#include <shared/world/WorldConstants.hpp>

namespace onion::voxel
{
	class SerializerDTO;
	class SerializerSave;

	class SubChunk
	{
		friend class SerializerDTO;
		friend class SerializerSave;

		// ----- Constructor / Destructor -----
	  public:
		SubChunk();
		~SubChunk();

		// ----- Public API -----
	  public:
		void Optimize();		  // Optimize the subchunk data (e.g., convert to mono block if all blocks are the same)
		bool IsEmpty();			  // Check if the subchunk is empty (all blocks are air)
		bool IsMonoBlock() const; // Check if the subchunk is made of a single block

		// ----- Getters / Setters -----
	  public:
		/// @brief Get the block index in the palette at the given local position within the subchunk
		/// @param localPosition The local position within the subchunk (0 to CHUNK_SIZE-1 in each axis)
		/// @return The block index in the palette at the given local position
		uint16_t GetBlockIndexInPalette(const glm::ivec3& localPosition) const;

		/// @brief Set the block index in the palette at the given local position within the subchunk
		/// @param localPosition The local position within the subchunk (0 to CHUNK_SIZE-1 in each axis)
		/// @param blockIndex The block index in the palette to set at the given local position
		void SetBlockIndexInPalette(const glm::ivec3& localPosition, uint16_t blockIndex);

		void
		SetBlockIndexInPalette_Unsafe(const uint8_t x, const uint8_t y, const uint8_t z, const uint16_t blockIndex);

		/// @brief Fill a contiguous y-range [yMin, yMax] in a single (x, z) column with the given palette index.
		/// All y values must be local to this SubChunk (0 to CHUNK_SIZE-1).
		void FillColumnRange_Unsafe(uint8_t x, uint8_t yMin, uint8_t yMax, uint8_t z, uint16_t blockIndex);

		// ----- Members -----
	  protected:
		bool m_IsMonoBlock = true;				// Whether the subchunk is made of a single block
		uint16_t m_MonoBlockIndexInPalette = 0; // The block data for the mono block (if m_IsMonoBlock is true)

		std::shared_ptr<
			std::array<uint16_t, WorldConstants::CHUNK_SIZE * WorldConstants::CHUNK_SIZE * WorldConstants::CHUNK_SIZE>>
			m_BlockIndexInPalette;
	};
} // namespace onion::voxel
