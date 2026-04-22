#include "BlockState.hpp"

namespace onion::voxel
{
	// ----- Static Initialization -----
	const std::vector<bool> BlockState::s_TransparencyLookupTable = []()
	{
		std::vector<bool> table(static_cast<size_t>(BlockIds::GetBlockIdCount()), false);
		table[static_cast<size_t>(BlockId::Air)] = true;
		table[static_cast<size_t>(BlockId::Glass)] = true;
		table[static_cast<size_t>(BlockId::BlackStainedGlass)] = true;
		table[static_cast<size_t>(BlockId::BlueStainedGlass)] = true;
		table[static_cast<size_t>(BlockId::BrownStainedGlass)] = true;
		table[static_cast<size_t>(BlockId::CyanStainedGlass)] = true;
		table[static_cast<size_t>(BlockId::GrayStainedGlass)] = true;
		table[static_cast<size_t>(BlockId::GreenStainedGlass)] = true;
		table[static_cast<size_t>(BlockId::LightBlueStainedGlass)] = true;
		table[static_cast<size_t>(BlockId::LightGrayStainedGlass)] = true;
		table[static_cast<size_t>(BlockId::LimeStainedGlass)] = true;
		table[static_cast<size_t>(BlockId::MagentaStainedGlass)] = true;
		table[static_cast<size_t>(BlockId::OrangeStainedGlass)] = true;
		table[static_cast<size_t>(BlockId::PinkStainedGlass)] = true;
		table[static_cast<size_t>(BlockId::PurpleStainedGlass)] = true;
		table[static_cast<size_t>(BlockId::RedStainedGlass)] = true;
		table[static_cast<size_t>(BlockId::WhiteStainedGlass)] = true;
		table[static_cast<size_t>(BlockId::YellowStainedGlass)] = true;
		table[static_cast<size_t>(BlockId::OakLeaves)] = true;
		table[static_cast<size_t>(BlockId::BirchLeaves)] = true;
		table[static_cast<size_t>(BlockId::SpruceLeaves)] = true;
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
		table[static_cast<size_t>(BlockId::Ice)] = true;
		table[static_cast<size_t>(BlockId::CactusFlower)] = true;
		return table;
	}();

	const std::vector<bool> BlockState::s_SolidLookupTable = []()
	{
		std::vector<bool> table(static_cast<size_t>(BlockIds::GetBlockIdCount()), true);
		table[static_cast<size_t>(BlockId::Air)] = false;
		table[static_cast<size_t>(BlockId::Water)] = false;
		table[static_cast<size_t>(BlockId::Kelp)] = false;
		table[static_cast<size_t>(BlockId::DeadBush)] = false;
		table[static_cast<size_t>(BlockId::OakSapling)] = false;
		table[static_cast<size_t>(BlockId::ShortGrass)] = false;
		table[static_cast<size_t>(BlockId::RedTulip)] = false;
		table[static_cast<size_t>(BlockId::OrangeTulip)] = false;
		table[static_cast<size_t>(BlockId::WhiteTulip)] = false;
		table[static_cast<size_t>(BlockId::PinkTulip)] = false;
		table[static_cast<size_t>(BlockId::Poppy)] = false;
		table[static_cast<size_t>(BlockId::Dandelion)] = false;
		return table;
	}();

	const std::vector<BlockId> BlockState::Flowers = {BlockId::Poppy,
													  BlockId::Dandelion,
													  BlockId::RedTulip,
													  BlockId::OrangeTulip,
													  BlockId::WhiteTulip,
													  BlockId::PinkTulip};

	bool BlockState::operator==(const BlockState& other) const
	{
		return ID == other.ID && VariantIndex == other.VariantIndex;
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

	bool BlockState::IsSolid(BlockId blockID)
	{
		return s_SolidLookupTable[static_cast<size_t>(blockID)];
	}

} // namespace onion::voxel
