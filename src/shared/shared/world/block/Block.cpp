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
		table[static_cast<size_t>(BlockId::Water)] = true;
		return table;
	}();

	const std::vector<Block::RotationType> Block::s_RotationTypeLookupTable = []()
	{
		std::vector<RotationType> table(static_cast<size_t>(GetBlockIdCount()), RotationType::None);
		table[static_cast<size_t>(BlockId::Furnace)] = RotationType::Horizontal;
		table[static_cast<size_t>(BlockId::OakLog)] = RotationType::Pillar;
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
		return !(*this == other);
	}

	bool Block::IsOpaque(BlockId blockID)
	{
		return !IsTransparent(blockID);
	}

	bool Block::IsTransparent(BlockId blockID)
	{
		return s_TransparencyLookupTable[static_cast<size_t>(blockID)];
	}

	Block::RotationType Block::GetRotationType(BlockId blockID)
	{
		return s_RotationTypeLookupTable[static_cast<size_t>(blockID)];
	}
} // namespace onion::voxel
