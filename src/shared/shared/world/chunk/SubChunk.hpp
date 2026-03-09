#pragma once

#include <glm/glm.hpp>

#include <array>
#include <memory>
#include <vector>

#include "../WorldConstants.hpp"

namespace onion::voxel
{
	class Serializer;

	class SubChunk
	{
		friend class Serializer;

		// ----- Constructor / Destructor -----
	  public:
		SubChunk();
		~SubChunk();

		// ----- Public API -----
	  public:
		void Optimize(); // Optimize the subchunk data (e.g., convert to mono block if all blocks are the same)
		bool IsEmpty();	 // Check if the subchunk is empty (all blocks are air)

		// ----- Getters / Setters -----
	  public:
		/// @brief Get the block index in the palette at the given local position within the subchunk
		/// @param localPosition The local position within the subchunk (0 to SUBCHUNK_SIZE-1 in each axis)
		/// @return The block index in the palette at the given local position
		uint8_t GetBlockIndexInPalette(const glm::ivec3& localPosition) const;

		/// @brief Set the block index in the palette at the given local position within the subchunk
		/// @param localPosition The local position within the subchunk (0 to SUBCHUNK_SIZE-1 in each axis)
		/// @param blockIndex The block index in the palette to set at the given local position
		void SetBlockIndexInPalette(const glm::ivec3& localPosition, uint8_t blockIndex);

		void SetBlockIndexInPalette_Unsafe(const uint8_t x, const uint8_t y, const uint8_t z, const uint8_t blockIndex);

		// ----- Members -----
	  protected:
		bool m_IsMonoBlock = true;			   // Whether the subchunk is made of a single block
		uint8_t m_MonoBlockIndexInPalette = 0; // The block data for the mono block (if m_IsMonoBlock is true)

		std::shared_ptr<
			std::array<uint8_t,
					   WorldConstants::SUBCHUNK_SIZE * WorldConstants::SUBCHUNK_SIZE * WorldConstants::SUBCHUNK_SIZE>>
			m_BlockIndexInPalette;
	};
} // namespace onion::voxel
