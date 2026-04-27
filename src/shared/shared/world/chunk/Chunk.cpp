#include "Chunk.hpp"

#include <algorithm>
#include <cassert>
#include <stdexcept>

namespace onion::voxel
{
	Chunk::Chunk(const glm::ivec2& position, const size_t subChunkCount) : m_Position(position)
	{
		m_SubChunks.resize(subChunkCount);
		m_ChunkHeight = static_cast<int>(subChunkCount * WorldConstants::CHUNK_SIZE);
	}

	Chunk::Chunk(glm::ivec2 position, std::vector<SubChunk> subChunks, std::vector<BlockState> blocksPalette)
		: m_Position(position), m_SubChunks(std::move(subChunks)), m_BlocksPalette(std::move(blocksPalette))
	{
		m_ChunkHeight = static_cast<int>(m_SubChunks.size() * WorldConstants::CHUNK_SIZE);
	}

	Chunk::~Chunk() {}

	void Chunk::Optimize()
	{
		std::unique_lock lock(m_Mutex);

		// Remove empty subchunks at the end of the subchunks vector
		while (!m_SubChunks.empty() && m_SubChunks.back().IsEmpty())
		{
			m_SubChunks.pop_back();
			m_ChunkHeight = static_cast<int>(m_SubChunks.size() * WorldConstants::CHUNK_SIZE);
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

	BlockState Chunk::GetBlock(const glm::ivec3& localPosition) const
	{
		assert(localPosition.x >= 0 && localPosition.x < WorldConstants::CHUNK_SIZE);
		assert(localPosition.z >= 0 && localPosition.z < WorldConstants::CHUNK_SIZE);

		std::shared_lock lock(m_Mutex);

		const int subChunkIndex = localPosition.y / WorldConstants::CHUNK_SIZE;

		// Checks subChunkIndex < 0 AND subChunkIndex >= size
		if (localPosition.y < 0 || subChunkIndex >= m_SubChunks.size())
		{
			return BlockState(BlockId::Air);
		}

		const SubChunk& subChunk = m_SubChunks[subChunkIndex];

		const glm::ivec3 local{localPosition.x, localPosition.y % WorldConstants::CHUNK_SIZE, localPosition.z};

		const uint16_t blockIndex = subChunk.GetBlockIndexInPalette(local);

		return m_BlocksPalette[blockIndex];
	}

	void Chunk::SetBlock(const glm::ivec3& localPosition, const BlockState& block)
	{
		// Check if localPosition is within bounds of the chunk
		// Check if localPosition is within bounds of the chunk
		assert(localPosition.x >= 0 && localPosition.x < WorldConstants::CHUNK_SIZE);
		assert(localPosition.y >= 0);
		assert(localPosition.z >= 0 && localPosition.z < WorldConstants::CHUNK_SIZE);

		std::unique_lock lock(m_Mutex);

		// Calculate which subchunk the local position is in
		int subChunkIndex = localPosition.y / WorldConstants::CHUNK_SIZE;

		// Fill the subchunks vector with empty subchunks until we have enough subchunks to fit the local position
		while (m_SubChunks.size() <= subChunkIndex)
		{
			m_SubChunks.emplace_back();
			m_ChunkHeight = static_cast<int>(m_SubChunks.size() * WorldConstants::CHUNK_SIZE);
		}

		// Add the block to the palette and get the block data index
		uint16_t indexInPalette = GetOrAddPaletteIndex(block);

		// Get the subchunk
		SubChunk& subChunk = m_SubChunks[subChunkIndex];

		// Calculate the local position within the subchunk
		glm::ivec3 localPositionInSubChunk{
			localPosition.x, localPosition.y % WorldConstants::CHUNK_SIZE, localPosition.z};

		// Set the block data in the subchunk
		subChunk.SetBlockIndexInPalette(localPositionInSubChunk, indexInPalette);
	}

	void voxel::Chunk::SetBlock_Unsafe(const uint8_t x, const uint16_t y, const uint8_t z, const BlockState& block)
	{
		// Add the block to the palette and get the block data index
		const uint16_t indexInPalette = GetOrAddPaletteIndex(block);

		// Calculate which subchunk the local position is in
		const int subChunkIndex = y / WorldConstants::CHUNK_SIZE;

		// Set the block data in the subchunk
		m_SubChunks[subChunkIndex].SetBlockIndexInPalette_Unsafe(x, y % WorldConstants::CHUNK_SIZE, z, indexInPalette);
	}

	int Chunk::GetSubChunkCount() const
	{
		std::shared_lock lock(m_Mutex);
		return static_cast<int>(m_SubChunks.size());
	}

	int Chunk::GetChunkHeight() const
	{
		return m_ChunkHeight;
	}

	int Chunk::GetHeightAt(int x, int z) const
	{
		BlockId selectedBlockId = GetBlock({x, m_ChunkHeight - 1, z}).ID;
		for (int i = m_ChunkHeight - 1; i >= 0; i--)
		{
			BlockId blockId = GetBlock({x, i, z}).ID;
			if (blockId != BlockId::Air)
			{
				return i + 1;
			}
		}

		return 0;
	}

	bool Chunk::IsSubchunkMonoBlock(const int subChunkIndex) const
	{
		std::shared_lock lock(m_Mutex);

		if (subChunkIndex < 0 || subChunkIndex >= m_SubChunks.size())
		{
			throw std::out_of_range("Subchunk index is out of bounds");
		}

		return m_SubChunks[subChunkIndex].IsMonoBlock();
	}

	uint16_t Chunk::GetOrAddPaletteIndex(const BlockState& block)
	{
		auto it = std::find(m_BlocksPalette.begin(), m_BlocksPalette.end(), block);

		if (it != m_BlocksPalette.end())
		{
			size_t index = std::distance(m_BlocksPalette.begin(), it);
			assert(index <= UINT16_MAX);
			return static_cast<uint16_t>(index);
		}

		m_BlocksPalette.push_back(block);
		size_t index = m_BlocksPalette.size() - 1;
		assert(index <= UINT16_MAX);
		return static_cast<uint16_t>(index);
	}

	void Chunk::FillColumn_Unsafe(uint8_t x, uint16_t yMin, uint16_t yMax, uint8_t z, uint16_t paletteIndex)
	{
		if (yMin > yMax)
			return;

		const int CS = WorldConstants::CHUNK_SIZE;
		const int firstSubChunk = yMin / CS;
		const int lastSubChunk = yMax / CS;

		for (int sci = firstSubChunk; sci <= lastSubChunk; ++sci)
		{
			uint8_t localYMin = (sci == firstSubChunk) ? static_cast<uint8_t>(yMin % CS) : 0;
			uint8_t localYMax = (sci == lastSubChunk) ? static_cast<uint8_t>(yMax % CS) : static_cast<uint8_t>(CS - 1);

			m_SubChunks[sci].FillColumnRange_Unsafe(x, localYMin, localYMax, z, paletteIndex);
		}
	}

} // namespace onion::voxel
