#include "BlockUpdater.hpp"

#include <shared/world/block/Block.hpp>
#include <shared/world/block/BlockIds.hpp>
#include <shared/world/block/BlockState.hpp>
#include <shared/world/block/BlockstateRegistry.hpp>
#include <shared/world/world_manager/WorldManager.hpp>

namespace
{
	using namespace onion::voxel;

	// Returns true if the fence gate at neighborState is oriented so that its
	// open mouth faces dirKey (the direction from the connecting block to the gate).
	//   facing=north/south → gate runs along Z → open mouths face north/south
	//   facing=east/west   → gate runs along X → open mouths face east/west
	static bool FenceGateConnectsFrom(const BlockState& neighborState, const std::string& dirKey)
	{
		const auto& registry = BlockstateRegistry::Get();
		auto it = registry.find(neighborState.ID);
		if (it == registry.end())
			return false;
		const auto& props = it->second[neighborState.VariantIndex].Properties;
		auto facingIt = props.find("facing");
		if (facingIt == props.end())
			return false;
		const std::string& facing = facingIt->second;

		if (facing == "north" || facing == "south")
			return dirKey == "east" || dirKey == "west";
		return dirKey == "north" || dirKey == "south";
	}
} // namespace

namespace onion::voxel
{
	// Neighbor index constants — matches the array contract in the header.
	static constexpr int kDown = 0;
	static constexpr int kUp = 1;
	static constexpr int kNorth = 2;
	static constexpr int kSouth = 3;
	static constexpr int kWest = 4;
	static constexpr int kEast = 5;

	// -------------------------------------------------------------------------
	// Public
	// -------------------------------------------------------------------------

	void BlockUpdater::Update(const BlockState& target,
							  const std::array<BlockState, 6>& neighbors,
							  const glm::ivec3& worldPos,
							  WorldManager& world)
	{
		UpdateMultiBlock(target, neighbors, worldPos, world);
		UpdateConnections(target, neighbors, worldPos, world);
	}

	// -------------------------------------------------------------------------
	// Private — multi-block integrity
	// -------------------------------------------------------------------------

	void BlockUpdater::UpdateMultiBlock(const BlockState& target,
										const std::array<BlockState, 6>& neighbors,
										const glm::ivec3& worldPos,
										WorldManager& world)
	{
		// Only relevant when the current cell was just cleared (set to Air).
		if (target.ID != BlockId::Air)
			return;

		// Check the block above: if it is a tall-plant upper half whose lower
		// half is now missing (this cell is Air), break it.
		const BlockState& above = neighbors[kUp];
		if (above.ID != BlockId::Air && BlockstateRegistry::IsTallPlant(above.ID))
		{
			const auto& registry = BlockstateRegistry::Get();
			auto it = registry.find(above.ID);
			if (it != registry.end())
			{
				const auto& variant = it->second[above.VariantIndex];
				auto halfIt = variant.Properties.find("half");
				if (halfIt != variant.Properties.end() && halfIt->second == "upper")
				{
					// The upper half lost its lower partner — break it.
					const glm::ivec3 abovePos = worldPos + glm::ivec3{0, 1, 0};
					world.SetBlock(Block(abovePos, BlockState(BlockId::Air)),
								   WorldManager::BlocksChangedEventArgs::eOrigin::ServerRequest,
								   /*notify=*/false);
				}
			}
		}

		// Check the block below: if it is a tall-plant lower half whose upper
		// half is now missing, break it.
		const BlockState& below = neighbors[kDown];
		if (below.ID != BlockId::Air && BlockstateRegistry::IsTallPlant(below.ID))
		{
			const auto& registry = BlockstateRegistry::Get();
			auto it = registry.find(below.ID);
			if (it != registry.end())
			{
				const auto& variant = it->second[below.VariantIndex];
				auto halfIt = variant.Properties.find("half");
				if (halfIt != variant.Properties.end() && halfIt->second == "lower")
				{
					// The lower half lost its upper partner — break it.
					const glm::ivec3 belowPos = worldPos + glm::ivec3{0, -1, 0};
					world.SetBlock(Block(belowPos, BlockState(BlockId::Air)),
								   WorldManager::BlocksChangedEventArgs::eOrigin::ServerRequest,
								   /*notify=*/false);
				}
			}
		}
	}

	// -------------------------------------------------------------------------
	// Private — connection state recomputation
	// -------------------------------------------------------------------------

	void BlockUpdater::UpdateConnections(const BlockState& target,
										 const std::array<BlockState, 6>& neighbors,
										 const glm::ivec3& worldPos,
										 WorldManager& world)
	{
		if (target.ID == BlockId::Air)
			return;

		// Horizontal neighbors in cardinal order used below.
		// Indices into the neighbors array:  N=2  S=3  W=4  E=5
		const BlockState* cardinals[4] = {
			&neighbors[kNorth],
			&neighbors[kSouth],
			&neighbors[kWest],
			&neighbors[kEast],
		};
		static const char* kCardinalKeys[4] = {"north", "south", "west", "east"};

		// -----------------------------------------------------------------
		// Fence gates — only update in_wall; preserve facing and open.
		// -----------------------------------------------------------------
		if (IsFenceGate(target.ID))
		{
			const auto& registry = BlockstateRegistry::Get();
			auto it = registry.find(target.ID);
			if (it == registry.end())
				return;

			const auto& currentProps = it->second[target.VariantIndex].Properties;

			auto facingIt = currentProps.find("facing");
			auto openIt = currentProps.find("open");
			if (facingIt == currentProps.end() || openIt == currentProps.end())
				return;

			const std::string& facing = facingIt->second;
			const std::string& open = openIt->second;

			// The two neighbors perpendicular to the gate's facing direction
			// are the ones that could be walls.
			// facing north/south → check west and east neighbors
			// facing east/west   → check north and south neighbors
			bool adjacentToWall = false;
			if (facing == "north" || facing == "south")
			{
				adjacentToWall = IsWall(neighbors[kWest].ID) || IsWall(neighbors[kEast].ID);
			}
			else // east or west
			{
				adjacentToWall = IsWall(neighbors[kNorth].ID) || IsWall(neighbors[kSouth].ID);
			}

			const std::string inWall = adjacentToWall ? "true" : "false";

			std::map<std::string, std::string> newProps = {
				{"facing", facing},
				{"open", open},
				{"in_wall", inWall},
			};

			const uint8_t newVariant = BlockstateRegistry::GetVariantIndex(target.ID, newProps);
			if (newVariant != target.VariantIndex)
			{
				world.SetBlock(Block(worldPos, BlockState(target.ID, newVariant)),
							   WorldManager::BlocksChangedEventArgs::eOrigin::ServerRequest,
							   /*notify=*/false);
			}
			return;
		}

		// -----------------------------------------------------------------
		// Walls — north/south/east/west = none/low, up = true/false
		// -----------------------------------------------------------------
		if (IsWall(target.ID))
		{
			int connectionCount = 0;
			bool connected[4] = {false, false, false, false};

			for (int i = 0; i < 4; ++i)
			{
				const BlockId nid = cardinals[i]->ID;
				connected[i] = IsWall(nid) || IsPane(nid) ||
					(IsFenceGate(nid) && FenceGateConnectsFrom(*cardinals[i], kCardinalKeys[i]));
				if (connected[i])
					++connectionCount;
			}

			// up=false only for a straight wall (exactly two opposite connections,
			// no others): N+S or W+E. Otherwise show the post (up=true).
			bool straightNS = connected[0] && connected[1] && !connected[2] && !connected[3];
			bool straightWE = connected[2] && connected[3] && !connected[0] && !connected[1];
			const std::string up = (straightNS || straightWE) ? "false" : "true";

			std::map<std::string, std::string> newProps;
			newProps["up"] = up;
			for (int i = 0; i < 4; ++i)
				newProps[kCardinalKeys[i]] = connected[i] ? "low" : "none";

			const uint8_t newVariant = BlockstateRegistry::GetVariantIndex(target.ID, newProps);
			if (newVariant != target.VariantIndex)
			{
				world.SetBlock(Block(worldPos, BlockState(target.ID, newVariant)),
							   WorldManager::BlocksChangedEventArgs::eOrigin::ServerRequest,
							   /*notify=*/false);
			}
			return;
		}

		// -----------------------------------------------------------------
		// Panes — north/south/east/west = true/false
		// Connect to other panes and walls.
		// -----------------------------------------------------------------
		if (IsPane(target.ID))
		{
			std::map<std::string, std::string> newProps;
			for (int i = 0; i < 4; ++i)
			{
				const BlockId nid = cardinals[i]->ID;
				newProps[kCardinalKeys[i]] = (IsPane(nid) || IsWall(nid)) ? "true" : "false";
			}

			const uint8_t newVariant = BlockstateRegistry::GetVariantIndex(target.ID, newProps);
			if (newVariant != target.VariantIndex)
			{
				world.SetBlock(Block(worldPos, BlockState(target.ID, newVariant)),
							   WorldManager::BlocksChangedEventArgs::eOrigin::ServerRequest,
							   /*notify=*/false);
			}
			return;
		}

		// -----------------------------------------------------------------
		// Fences — north/south/east/west = true/false
		// Connect to other fences and fence gates.
		// -----------------------------------------------------------------
		if (IsConnectableBlock(target.ID))
		{
			std::map<std::string, std::string> newProps;
			for (int i = 0; i < 4; ++i)
			{
				const BlockId nid = cardinals[i]->ID;
				newProps[kCardinalKeys[i]] =
					(IsConnectableBlock(nid) && !IsWall(nid) && !IsPane(nid) ||
					 (IsFenceGate(nid) && FenceGateConnectsFrom(*cardinals[i], kCardinalKeys[i])))
					? "true"
					: "false";
			}

			const uint8_t newVariant = BlockstateRegistry::GetVariantIndex(target.ID, newProps);
			if (newVariant != target.VariantIndex)
			{
				world.SetBlock(Block(worldPos, BlockState(target.ID, newVariant)),
							   WorldManager::BlocksChangedEventArgs::eOrigin::ServerRequest,
							   /*notify=*/false);
			}
		}
	}

	// -------------------------------------------------------------------------
	// Private — detection helpers
	// -------------------------------------------------------------------------

	bool BlockUpdater::BlockHasProperty(BlockId id, const std::string& key)
	{
		const auto& registry = BlockstateRegistry::Get();
		auto it = registry.find(id);
		if (it == registry.end())
			return false;

		for (const auto& variant : it->second)
		{
			if (variant.Properties.count(key) > 0)
				return true;
		}
		return false;
	}

	bool BlockUpdater::IsConnectableBlock(BlockId id)
	{
		return BlockHasProperty(id, "north");
	}

	bool BlockUpdater::IsWall(BlockId id)
	{
		if (!IsConnectableBlock(id))
			return false;
		const std::string& name = BlockIds::GetName(id);
		return name.find("Wall") != std::string::npos;
	}

	bool BlockUpdater::IsPane(BlockId id)
	{
		if (!IsConnectableBlock(id))
			return false;
		const std::string& name = BlockIds::GetName(id);
		return name.find("Pane") != std::string::npos;
	}

	bool BlockUpdater::IsFenceGate(BlockId id)
	{
		return BlockHasProperty(id, "in_wall");
	}

} // namespace onion::voxel
