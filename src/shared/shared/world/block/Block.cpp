#include "Block.hpp"

namespace onion::voxel
{
	bool Block::operator==(const Block& other) const
	{
		return m_BlockID == other.m_BlockID && m_Orientation == other.m_Orientation;
	}

	bool Block::operator!=(const Block& other) const
	{
		return false;
	}
} // namespace onion::voxel
