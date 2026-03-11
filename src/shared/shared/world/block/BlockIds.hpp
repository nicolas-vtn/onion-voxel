#pragma once

namespace onion::voxel
{
	/// @brief The block IDs for all the blocks in the game
	enum class BlockId : uint8_t
	{
		Air = 0,
		Dirt = 1,
		Grass = 2,
		Stone = 3,
		Glass = 4,
		OakLog = 5,
		OakLeaves = 6,
		Furnace = 7,
		Bedrock = 8,
		Water = 9,
		Sand = 10,
		Gravel = 11,
		Cobblestone = 12,
	};

	inline int GetBlockIdCount()
	{
		return 13;
	}

} // namespace onion::voxel
