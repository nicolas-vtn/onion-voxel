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
		uint8_t firstBlockIndex = (*m_BlockIndexInPalette)[0];

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

	uint8_t SubChunk::GetBlockIndexInPalette(const glm::ivec3& localPosition) const
	{
		if (localPosition.x < 0 || localPosition.x >= WorldConstants::CHUNK_SIZE || localPosition.y < 0 ||
			localPosition.y >= WorldConstants::CHUNK_SIZE || localPosition.z < 0 ||
			localPosition.z >= WorldConstants::CHUNK_SIZE)
		{
			throw std::out_of_range("SubChunk::GetBlock: Local position is out of bounds");
		}

		if (m_IsMonoBlock)
		{
			return m_MonoBlockIndexInPalette; // Return the mono block index in the palette
		}

		if (!m_BlockIndexInPalette)
		{
			throw std::runtime_error("SubChunk::GetBlock: Block data is not initialized");
		}

		const size_t index = localPosition.x + localPosition.y * WorldConstants::CHUNK_SIZE +
			localPosition.z * WorldConstants::CHUNK_SIZE * WorldConstants::CHUNK_SIZE;

		return (*m_BlockIndexInPalette)[index];
	}

	void SubChunk::SetBlockIndexInPalette(const glm::ivec3& localPosition, uint8_t blockIndex)
	{

		if (localPosition.x < 0 || localPosition.x >= WorldConstants::CHUNK_SIZE || localPosition.y < 0 ||
			localPosition.y >= WorldConstants::CHUNK_SIZE || localPosition.z < 0 ||
			localPosition.z >= WorldConstants::CHUNK_SIZE)
		{
			throw std::out_of_range("SubChunk::SetBlock: Local position is out of bounds");
		}

		if (m_IsMonoBlock)
		{
			if (blockIndex == m_MonoBlockIndexInPalette)
			{
				return; // No change needed
			}

			// Need to convert to non-mono block data
			m_BlockIndexInPalette = std::make_shared<
				std::array<uint8_t,
						   WorldConstants::CHUNK_SIZE * WorldConstants::CHUNK_SIZE * WorldConstants::CHUNK_SIZE>>();

			m_BlockIndexInPalette->fill(m_MonoBlockIndexInPalette); // Fill with the mono block index
			m_IsMonoBlock = false;									// No longer a mono block
		}

		if (!m_BlockIndexInPalette)
		{
			throw std::runtime_error("SubChunk::SetBlock: Block data is not initialized");
		}

		const size_t index = localPosition.x + localPosition.y * WorldConstants::CHUNK_SIZE +
			localPosition.z * WorldConstants::CHUNK_SIZE * WorldConstants::CHUNK_SIZE;

		(*m_BlockIndexInPalette)[index] = blockIndex;
	}

	void voxel::SubChunk::SetBlockIndexInPalette_Unsafe(const uint8_t x,
														const uint8_t y,
														const uint8_t z,
														const uint8_t blockIndex)
	{
		if (m_IsMonoBlock)
		{
			if (blockIndex == m_MonoBlockIndexInPalette)
			{
				return; // No change needed
			}

			// Need to convert to non-mono block data
			m_BlockIndexInPalette = std::make_shared<
				std::array<uint8_t,
						   WorldConstants::CHUNK_SIZE * WorldConstants::CHUNK_SIZE * WorldConstants::CHUNK_SIZE>>();

			m_BlockIndexInPalette->fill(m_MonoBlockIndexInPalette); // Fill with the mono block index
			m_IsMonoBlock = false;									// No longer a mono block
		}

		const size_t index =
			x + y * WorldConstants::CHUNK_SIZE + z * WorldConstants::CHUNK_SIZE * WorldConstants::CHUNK_SIZE;

		(*m_BlockIndexInPalette)[index] = blockIndex;
	}

} // namespace onion::voxel
