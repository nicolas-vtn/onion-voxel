#pragma once

#include <glm/glm.hpp>

#include <vector>

#include "BlockIds.hpp"

namespace onion::voxel
{
	class BlockState
	{
		// ----- Enums -----
	  public:
		enum class Orientation
		{
			None,
			Up,
			Down,
			North,
			South,
			East,
			West
		};

		enum class RotationType
		{
			None,		// Dirt, stone
			Horizontal, // Furnace, chest
			Pillar,		// Rotatable along X/Y/Z
			Facing,		// Observers, pistons
		};

		// ----- Constructor / Destructor -----
	  public:
		BlockState() = default;
		BlockState(BlockId blockID, Orientation facing = Orientation::None, Orientation top = Orientation::None);
		~BlockState() = default;

		// ----- Operators -----
	  public:
		bool operator==(const BlockState& other) const;
		bool operator!=(const BlockState& other) const;

		// ----- Members -----
	  public:
		BlockId ID = BlockId::Air; // The block ID (type) of this block
		Orientation Facing = Orientation::None;
		Orientation Top = Orientation::None;

		// ----- Static Helpers -----
	  public:
		static bool IsOpaque(BlockId blockID);
		static bool IsTransparent(BlockId blockID);
		static bool IsSolid(BlockId blockID);
		static RotationType GetRotationType(BlockId blockID);

		// ----- Static Members -----
	  private:
		// A lookup table for block transparency, indexed by BlockId
		static const std::vector<bool> s_TransparencyLookupTable;

		// A lookup table for block rotation types, indexed by BlockId
		static const std::vector<RotationType> s_RotationTypeLookupTable;

		// A lookup table for block solidity, indexed by BlockId
		static const std::vector<bool> s_SolidLookupTable;

		// ----- Lists of block IDs for different categories -----
	  public:
		static const std::vector<BlockId> Flowers;
	};
} // namespace onion::voxel
