#pragma once

namespace onion::voxel
{
	/// @brief The block IDs for all the blocks in the game
	enum class BlockId : uint8_t
	{
		Air = 0,
		Dirt,
		Grass,
		SnowGrass,
		SnowBlock,
		Stone,
		Glass,
		OakLog,
		OakLeaves,
		BirchLog,
		BirchLeaves,
		SpruceLog,
		SpruceLeaves,
		Furnace,
		Bedrock,
		Water,
		Ice,
		Sand,
		Sandstone,
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
		Cactus,
		CactusFlower,

		Count,
	};

	inline int GetBlockIdCount()
	{
		return static_cast<int>(BlockId::Count);
	}

} // namespace onion::voxel
