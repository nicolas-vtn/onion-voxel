#pragma once

#include <glm/glm.hpp>

#include <vector>

#include "BlockIds.hpp"

namespace onion::voxel
{
	class Block
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
		Block() = default;
		Block(BlockId blockID, Orientation facing = Orientation::None, Orientation top = Orientation::None);
		~Block() = default;

		// ----- Operators -----
	  public:
		bool operator==(const Block& other) const;
		bool operator!=(const Block& other) const;

		// ----- Members -----
	  public:
		BlockId m_BlockID = BlockId::Air; // The block ID (type) of this block
		Orientation m_Facing = Orientation::None;
		Orientation m_Top = Orientation::None;

		// ----- Static Helpers -----
	  public:
		static bool IsTransparent(BlockId blockID);
		static RotationType GetRotationType(BlockId blockID);

		// ----- Static Members -----
	  private:
		static const std::vector<bool>
			s_TransparencyLookupTable; // A lookup table for block transparency, indexed by BlockId
		static const std::vector<RotationType>
			s_RotationTypeLookupTable; // A lookup table for block rotation types, indexed by BlockId
	};
} // namespace onion::voxel
