#include "SubChunk.hpp"

#include <stdexcept>

namespace onion::voxel
{
	SubChunk::SubChunk() {}

	SubChunk::~SubChunk() {}

	void SubChunk::Optimize()
	{
		if (m_IsMonoBlock)
		{
			return; // Already optimized
		}

		if (!m_BlockIndexInPalette)
		{
			throw std::runtime_error("SubChunk::Optimize: Block data is not initialized");
		}

		// Check if all blocks are the same
		uint16_t firstBlockIndex = (*m_BlockIndexInPalette)[0];

		for (const auto& blockIndex : *m_BlockIndexInPalette)
		{
			if (blockIndex != firstBlockIndex)
			{
				return; // Not all blocks are the same, cannot optimize
			}
		}

		// All blocks are the same, optimize to mono block
		m_IsMonoBlock = true;
		m_MonoBlockIndexInPalette = firstBlockIndex;
	}

	bool SubChunk::IsEmpty()
	{
		if (!m_IsMonoBlock)
			Optimize();

		return m_IsMonoBlock && m_MonoBlockIndexInPalette == 0;
	}

	bool voxel::SubChunk::IsMonoBlock() const
	{
		return m_IsMonoBlock;
	}

	uint16_t SubChunk::GetBlockIndexInPalette(const glm::ivec3& localPosition) const
	{
		// Check if the local position is within bounds
		assert(localPosition.x >= 0 && localPosition.x < WorldConstants::CHUNK_SIZE);
		assert(localPosition.y >= 0 && localPosition.y < WorldConstants::CHUNK_SIZE);
		assert(localPosition.z >= 0 && localPosition.z < WorldConstants::CHUNK_SIZE);

		if (m_IsMonoBlock)
		{
			return m_MonoBlockIndexInPalette; // Return the mono block index in the palette
		}

		assert(m_BlockIndexInPalette);

		const size_t index = localPosition.x + localPosition.y * WorldConstants::CHUNK_SIZE +
			localPosition.z * WorldConstants::CHUNK_SIZE * WorldConstants::CHUNK_SIZE;

		return (*m_BlockIndexInPalette)[index];
	}

	void SubChunk::SetBlockIndexInPalette(const glm::ivec3& localPosition, uint16_t blockIndex)
	{
		// Check if the local position is within bounds
		assert(localPosition.x >= 0 && localPosition.x < WorldConstants::CHUNK_SIZE);
		assert(localPosition.y >= 0 && localPosition.y < WorldConstants::CHUNK_SIZE);
		assert(localPosition.z >= 0 && localPosition.z < WorldConstants::CHUNK_SIZE);

		if (m_IsMonoBlock)
		{
			if (blockIndex == m_MonoBlockIndexInPalette)
			{
				return; // No change needed
			}

			// Need to convert to non-mono block data
			m_BlockIndexInPalette = std::make_shared<
				std::array<uint16_t,
						   WorldConstants::CHUNK_SIZE * WorldConstants::CHUNK_SIZE * WorldConstants::CHUNK_SIZE>>();

			m_BlockIndexInPalette->fill(m_MonoBlockIndexInPalette); // Fill with the mono block index
			m_IsMonoBlock = false;									// No longer a mono block
		}

		assert(m_BlockIndexInPalette);

		const size_t index = localPosition.x + localPosition.y * WorldConstants::CHUNK_SIZE +
			localPosition.z * WorldConstants::CHUNK_SIZE * WorldConstants::CHUNK_SIZE;

		(*m_BlockIndexInPalette)[index] = blockIndex;
	}

	void voxel::SubChunk::SetBlockIndexInPalette_Unsafe(const uint8_t x,
														const uint8_t y,
														const uint8_t z,
														const uint16_t blockIndex)
	{
		if (m_IsMonoBlock)
		{
			if (blockIndex == m_MonoBlockIndexInPalette)
			{
				return; // No change needed
			}

			// Need to convert to non-mono block data
			m_BlockIndexInPalette = std::make_shared<
				std::array<uint16_t,
						   WorldConstants::CHUNK_SIZE * WorldConstants::CHUNK_SIZE * WorldConstants::CHUNK_SIZE>>();

			m_BlockIndexInPalette->fill(m_MonoBlockIndexInPalette); // Fill with the mono block index
			m_IsMonoBlock = false;									// No longer a mono block
		}

		const size_t index =
			x + y * WorldConstants::CHUNK_SIZE + z * WorldConstants::CHUNK_SIZE * WorldConstants::CHUNK_SIZE;

		(*m_BlockIndexInPalette)[index] = blockIndex;
	}

	void SubChunk::FillColumnRange_Unsafe(uint8_t x, uint8_t yMin, uint8_t yMax, uint8_t z, uint16_t blockIndex)
	{
		if (m_IsMonoBlock)
		{
			if (blockIndex == m_MonoBlockIndexInPalette)
				return; // No change needed

			m_BlockIndexInPalette = std::make_shared<
				std::array<uint16_t,
						   WorldConstants::CHUNK_SIZE * WorldConstants::CHUNK_SIZE * WorldConstants::CHUNK_SIZE>>();

			m_BlockIndexInPalette->fill(m_MonoBlockIndexInPalette);
			m_IsMonoBlock = false;
		}

		const size_t stride = WorldConstants::CHUNK_SIZE; // y-stride in the flat array
		const size_t base = x + z * WorldConstants::CHUNK_SIZE * WorldConstants::CHUNK_SIZE;

		uint16_t* data = m_BlockIndexInPalette->data();
		for (uint8_t y = yMin; y <= yMax; ++y)
			data[base + y * stride] = blockIndex;
	}

} // namespace onion::voxel
