#pragma once

namespace onion::voxel
{
	/// @brief The block IDs for all the blocks in the game
	enum class BlockId : uint8_t
	{
		Air = 0,
		Dirt,
		Grass,
		Stone,
		Glass,
		OakLog,
		OakLeaves,
		Furnace,
		Bedrock,
		Water,
		Sand,
		Gravel,
		Cobblestone,
		Poppy,
		Dandelion,
		BrownMushroom,
		RedMushroom,
		Cobweb,
		Kelp,
		DeadBush,
		OakSapling,
		ShortGrass,
		RedTulip,
		OrangeTulip,
		WhiteTulip,
		PinkTulip,
	};

	inline int GetBlockIdCount()
	{
		return 26;
	}

} // namespace onion::voxel
