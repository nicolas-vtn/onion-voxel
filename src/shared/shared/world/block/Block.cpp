#include "Block.hpp"

namespace onion::voxel
{
	Block::Block() : Position(0), State(BlockState(BlockId::Air)) {}

	Block::Block(const glm::ivec3& position, const BlockState& state) : Position(position), State(state) {}

	bool Block::operator==(const Block& other) const
	{
		return Position == other.Position && State == other.State;
	}

	bool Block::operator!=(const Block& other) const
	{
		return !(*this == other);
	}

	bool Block::IsTransparent() const
	{
		return BlockState::IsTransparent(State.ID);
	}

} // namespace onion::voxel
