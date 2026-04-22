#pragma once

#include <glm/glm.hpp>

#include "BlockIds.hpp"
#include "BlockState.hpp"

namespace onion::voxel
{
	class Block
	{
		// ----- Constructor / Destructor -----
	  public:
		Block();
		Block(const glm::ivec3& position, const BlockState& state);
		~Block() = default;

		// ----- Operators -----
	  public:
		bool operator==(const Block& other) const;
		bool operator!=(const Block& other) const;

		// ----- Members -----
	  public:
		glm::ivec3 Position{};
		BlockState State{};

		// ----- Getters -----
	  public:
		bool IsOpaque() const;
		bool IsTransparent() const;

		BlockId ID() const { return State.ID; }
	};
} // namespace onion::voxel
