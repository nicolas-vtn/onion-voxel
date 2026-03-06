#include "Block.hpp"

namespace onion::voxel
{

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
} // namespace onion::voxel
