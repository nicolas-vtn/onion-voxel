#include "Block.hpp"

namespace onion::voxel
{
	// ----- Static Initialization -----
	const std::vector<bool> Block::s_TransparencyLookupTable = []()
	{
		std::vector<bool> table(static_cast<size_t>(GetBlockIdCount()), false);
		table[static_cast<size_t>(BlockId::Air)] = true;
		table[static_cast<size_t>(BlockId::Glass)] = true;
		table[static_cast<size_t>(BlockId::OakLeaves)] = true;
		return table;
	}();

	Block::Block(BlockId blockID, Orientation facing, Orientation top)
		: m_BlockID(blockID), m_Facing(facing), m_Top(top)
	{
	}

	bool Block::operator==(const Block& other) const
	{
		return m_BlockID == other.m_BlockID && m_Facing == other.m_Facing && m_Top == other.m_Top;
	}

	bool Block::operator!=(const Block& other) const
	{
		return false;
	}

	bool Block::IsTransparent(BlockId blockID)
	{
		return s_TransparencyLookupTable[static_cast<size_t>(blockID)];
	}
} // namespace onion::voxel
