#pragma once

#include <glm/glm.hpp>

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

		// ----- Constructor / Destructor -----
	  public:
		Block() = default;
		~Block() = default;

		// ----- Operators -----
	  public:
		bool operator==(const Block& other) const;
		bool operator!=(const Block& other) const;

		// ----- Members -----
	  public:
		BlockId m_BlockID = BlockId::Air; // The block ID (type) of this block
		Orientation m_Orientation =
			Orientation::None; // The orientation of the block (for blocks that have different orientations)
	};
} // namespace onion::voxel
