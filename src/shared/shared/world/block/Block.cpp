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

	bool Block::IsTransparent(BlockId blockID)
	{
		return std::find(s_TransparentBlocks.begin(), s_TransparentBlocks.end(), blockID) != s_TransparentBlocks.end();
	}
} // namespace onion::voxel
