#pragma once

#include <array>
#include <string>

#include <glm/glm.hpp>

#include <shared/world/block/BlockId.hpp>
#include <shared/world/block/BlockState.hpp>

namespace onion::voxel
{
	class WorldManager;

	/// Handles world block-update logic: multi-block structure integrity and
	/// connected blockstate recomputation (fences, walls, panes, fence gates).
	///
	/// Intentionally separate from WorldManager to keep block-update logic
	/// self-contained and testable without pulling in the full world context.
	///
	/// Neighbor order (matches Face enum — Down, Up, North, South, West, East):
	///   [0] = -Y (Down)
	///   [1] = +Y (Up)
	///   [2] = -Z (North)
	///   [3] = +Z (South)
	///   [4] = -X (West)
	///   [5] = +X (East)
	class BlockUpdater
	{
	  public:
		/// Runs all block-update behaviors for the block at worldPos.
		/// @param target    The block currently at worldPos.
		/// @param neighbors The 6 face-adjacent neighbors in Down/Up/North/South/West/East order.
		/// @param worldPos  World-space position of the target block.
		/// @param world     Used to issue SetBlock calls when the block state must change.
		static void Update(const BlockState& target,
						   const std::array<BlockState, 6>& neighbors,
						   const glm::ivec3& worldPos,
						   WorldManager& world);

	  private:
		/// Breaks orphaned halves of tall-plant structures (e.g. tall flowers, tall grass).
		static void UpdateMultiBlock(const BlockState& target,
									 const std::array<BlockState, 6>& neighbors,
									 const glm::ivec3& worldPos,
									 WorldManager& world);

		/// Recomputes connection properties for fences, walls, panes and fence gates.
		static void UpdateConnections(const BlockState& target,
									  const std::array<BlockState, 6>& neighbors,
									  const glm::ivec3& worldPos,
									  WorldManager& world);

		// ----- Detection helpers -----
	  private:
		/// True if any variant of id has the given property key.
		static bool BlockHasProperty(BlockId id, const std::string& key);

		/// True if the block has cardinal connection properties (north/south/east/west).
		/// Covers fences, walls, and panes.
		static bool IsConnectableBlock(BlockId id);

		/// True if the block is a wall (IsConnectableBlock AND name contains "wall").
		static bool IsWall(BlockId id);

		/// True if the block is a pane (IsConnectableBlock AND name contains "pane").
		static bool IsPane(BlockId id);

		/// True if the block has an "in_wall" property (fence gates).
		static bool IsFenceGate(BlockId id);

		/// True if the block has a "shape" property with stair values (stairs).
		static bool IsStair(BlockId id);

		/// Returns the facing of a stair neighbor if it is a stair with the same half,
		/// or an empty string if the neighbor is not a qualifying stair.
		static std::string GetStairFacing(const BlockState& neighbor, const std::string& requiredHalf);
	};
} // namespace onion::voxel
