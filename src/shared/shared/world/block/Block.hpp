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
	};
} // namespace onion::voxel
