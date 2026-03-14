#include "BlockState.hpp"

namespace onion::voxel
{
	// ----- Static Initialization -----
	const std::vector<bool> BlockState::s_TransparencyLookupTable = []()
	{
		std::vector<bool> table(static_cast<size_t>(GetBlockIdCount()), false);
		table[static_cast<size_t>(BlockId::Air)] = true;
		table[static_cast<size_t>(BlockId::Glass)] = true;
		table[static_cast<size_t>(BlockId::OakLeaves)] = true;
		table[static_cast<size_t>(BlockId::BirchLeaves)] = true;
		table[static_cast<size_t>(BlockId::Water)] = true;
		table[static_cast<size_t>(BlockId::Poppy)] = true;
		table[static_cast<size_t>(BlockId::Dandelion)] = true;
		table[static_cast<size_t>(BlockId::BrownMushroom)] = true;
		table[static_cast<size_t>(BlockId::RedMushroom)] = true;
		table[static_cast<size_t>(BlockId::Cobweb)] = true;
		table[static_cast<size_t>(BlockId::Kelp)] = true;
		table[static_cast<size_t>(BlockId::DeadBush)] = true;
		table[static_cast<size_t>(BlockId::OakSapling)] = true;
		table[static_cast<size_t>(BlockId::ShortGrass)] = true;
		table[static_cast<size_t>(BlockId::RedTulip)] = true;
		table[static_cast<size_t>(BlockId::OrangeTulip)] = true;
		table[static_cast<size_t>(BlockId::WhiteTulip)] = true;
		table[static_cast<size_t>(BlockId::PinkTulip)] = true;
		return table;
	}();

	const std::vector<BlockState::RotationType> BlockState::s_RotationTypeLookupTable = []()
	{
		std::vector<RotationType> table(static_cast<size_t>(GetBlockIdCount()), RotationType::None);
		table[static_cast<size_t>(BlockId::Furnace)] = RotationType::Horizontal;
		table[static_cast<size_t>(BlockId::OakLog)] = RotationType::Pillar;
		return table;
	}();

	const std::vector<BlockId> BlockState::Flowers = {BlockId::Poppy,
													  BlockId::Dandelion,
													  BlockId::RedTulip,
													  BlockId::OrangeTulip,
													  BlockId::WhiteTulip,
													  BlockId::PinkTulip};

	// ----- Constructor / Destructor -----

	BlockState::BlockState(BlockId blockID, Orientation facing, Orientation top) : ID(blockID), Facing(facing), Top(top)
	{
	}

	bool BlockState::operator==(const BlockState& other) const
	{
		return ID == other.ID && Facing == other.Facing && Top == other.Top;
	}

	bool BlockState::operator!=(const BlockState& other) const
	{
		return !(*this == other);
	}

	bool BlockState::IsOpaque(BlockId blockID)
	{
		return !IsTransparent(blockID);
	}

	bool BlockState::IsTransparent(BlockId blockID)
	{
		return s_TransparencyLookupTable[static_cast<size_t>(blockID)];
	}

	BlockState::RotationType BlockState::GetRotationType(BlockId blockID)
	{
		return s_RotationTypeLookupTable[static_cast<size_t>(blockID)];
	}
} // namespace onion::voxel
