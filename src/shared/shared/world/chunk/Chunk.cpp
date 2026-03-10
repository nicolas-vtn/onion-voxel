#include "Chunk.hpp"

#include <stdexcept>

namespace onion::voxel
{
	Chunk::Chunk(const glm::ivec2& position, const size_t subChunkCount) : m_Position(position)
	{
		m_SubChunks.resize(subChunkCount);
	}

	Chunk::Chunk(glm::ivec2 position, std::vector<SubChunk> subChunks, std::vector<Block> blocksPalette)
		: m_Position(position), m_SubChunks(std::move(subChunks)), m_BlocksPalette(std::move(blocksPalette))
	{
	}

	Chunk::~Chunk() {}

	void Chunk::Optimize()
	{
		std::unique_lock lock(m_Mutex);

		// Remove empty subchunks at the end of the subchunks vector
		while (!m_SubChunks.empty() && m_SubChunks.back().IsEmpty())
		{
			m_SubChunks.pop_back();
		}

		// Optimize each subchunk
		for (SubChunk& subChunk : m_SubChunks)
		{
			subChunk.Optimize();
		}
	}

	glm::ivec2 Chunk::GetPosition() const
	{
		return m_Position;
	}

	Block Chunk::GetBlock(const glm::ivec3& localPosition) const
	{
		// Check if localPosition is within bounds of the chunk
		if (localPosition.x < 0 || localPosition.x >= WorldConstants::CHUNK_SIZE || localPosition.y < 0 ||
			localPosition.z < 0 || localPosition.z >= WorldConstants::CHUNK_SIZE)
		{
			// Out of bounds, throw
			throw std::out_of_range("Local position is out of bounds of the chunk");
		}

		std::shared_lock lock(m_Mutex);

		// Calculate which subchunk the local position is in
		int subChunkIndex = localPosition.y / WorldConstants::CHUNK_SIZE;

		if (subChunkIndex >= m_SubChunks.size())
		{
			return m_BlocksPalette[0]; // Air
		}

		// Get the subchunk
		const SubChunk& subChunk = m_SubChunks[subChunkIndex];

		// Calculate the local position within the subchunk
		glm::ivec3 localPositionInSubChunk{
			localPosition.x, localPosition.y % WorldConstants::CHUNK_SIZE, localPosition.z};

		// Get the block data from the subchunk
		uint8_t blockIndex = subChunk.GetBlockIndexInPalette(localPositionInSubChunk);

		// Convert block data to Block and return
		return m_BlocksPalette[blockIndex];
	}

	void Chunk::SetBlock(const glm::ivec3& localPosition, const Block& block)
	{
		// Check if localPosition is within bounds of the chunk
		if (localPosition.x < 0 || localPosition.x >= WorldConstants::CHUNK_SIZE || localPosition.y < 0 ||
			localPosition.z < 0 || localPosition.z >= WorldConstants::CHUNK_SIZE)
		{
			// Out of bounds, throw
			throw std::out_of_range("Local position is out of bounds of the chunk");
		}

		std::unique_lock lock(m_Mutex);

		// Fill the subchunks vector with empty subchunks until we have enough subchunks to fit the local position
		while (m_SubChunks.size() <= localPosition.y / WorldConstants::CHUNK_SIZE)
		{
			m_SubChunks.emplace_back();
		}

		// Add the block to the palette and get the block data index
		uint8_t indexInPalette = AddBlockToPalette(block);

		// Calculate which subchunk the local position is in
		int subChunkIndex = localPosition.y / WorldConstants::CHUNK_SIZE;

		// Get the subchunk
		SubChunk& subChunk = m_SubChunks[subChunkIndex];

		// Calculate the local position within the subchunk
		glm::ivec3 localPositionInSubChunk{
			localPosition.x, localPosition.y % WorldConstants::CHUNK_SIZE, localPosition.z};

		// Set the block data in the subchunk
		subChunk.SetBlockIndexInPalette(localPositionInSubChunk, indexInPalette);
	}

	void voxel::Chunk::SetBlock_Unsafe(const uint8_t x, const uint8_t y, const uint8_t z, const Block& block)
	{
		// Add the block to the palette and get the block data index
		const uint8_t indexInPalette = AddBlockToPalette(block);

		// Calculate which subchunk the local position is in
		const int subChunkIndex = y / WorldConstants::CHUNK_SIZE;

		// Set the block data in the subchunk
		m_SubChunks[subChunkIndex].SetBlockIndexInPalette_Unsafe(
			x, y % WorldConstants::CHUNK_SIZE, z, indexInPalette);
	}

	int Chunk::GetSubChunkCount() const
	{
		std::shared_lock lock(m_Mutex);
		return static_cast<int>(m_SubChunks.size());
	}

	uint8_t Chunk::AddBlockToPalette(const Block& block)
	{
		// Checks if the block is already in the block palette, if not add it and get the new block data index
		auto it = std::find(m_BlocksPalette.begin(), m_BlocksPalette.end(), block);

		uint8_t indexInPalette;

		if (it != m_BlocksPalette.end())
		{
			indexInPalette = std::distance(m_BlocksPalette.begin(), it);
		}
		else
		{
			m_BlocksPalette.push_back(block);
			indexInPalette = m_BlocksPalette.size() - 1;
		}

		return indexInPalette;
	}

} // namespace onion::voxel
