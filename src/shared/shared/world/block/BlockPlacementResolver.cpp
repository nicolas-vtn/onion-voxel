#include "BlockPlacementResolver.hpp"

#include <shared/world/block/BlockstateRegistry.hpp>

namespace onion::voxel
{
	// -------------------------------------------------------------------------
	// Public
	// -------------------------------------------------------------------------

	std::map<std::string, std::string> BlockPlacementResolver::Resolve(const PlacementContext& ctx)
	{
		std::map<std::string, std::string> props;

		ResolveDirectionalFacing(ctx, props);
		ResolveAxis(ctx, props);
		ResolveHalf(ctx, props);
		ResolveType(ctx, props);

		return props;
	}

	// -------------------------------------------------------------------------
	// Private
	// -------------------------------------------------------------------------

	void BlockPlacementResolver::ResolveDirectionalFacing(const PlacementContext& ctx,
														  std::map<std::string, std::string>& props)
	{
		if (!BlockHasProperty(ctx.Id, "facing"))
			return;

		// Some blocks (hoppers, observers, droppers, dispensers) can also face
		// up or down. Check if those variants exist.
		bool supportsVertical = BlockHasProperty(ctx.Id, "facing") && [&]()
		{
			const auto& registry = BlockstateRegistry::Get();
			auto it = registry.find(ctx.Id);
			if (it == registry.end())
				return false;
			for (const auto& variant : it->second)
			{
				auto facingIt = variant.Properties.find("facing");
				if (facingIt != variant.Properties.end())
				{
					if (facingIt->second == "up" || facingIt->second == "down")
						return true;
				}
			}
			return false;
		}();

		// --- Vertical facing (only for blocks that support up/down) ---
		if (supportsVertical)
		{
			// Use the hit face normal: if the player hit the top face of a block,
			// the block is placed above it → the placed block faces down (toward
			// the block it was placed on). This matches Minecraft's hopper behaviour.
			if (ctx.HitFaceNormal.y == 1)
			{
				props["facing"] = "down";
				return;
			}
			if (ctx.HitFaceNormal.y == -1)
			{
				props["facing"] = "up";
				return;
			}

			// Also check player pitch for steep looks when placing on side faces.
			// If looking steeply up/down (pitch > 45°), prefer vertical.
			// PlayerLookDir.y: -1 = straight down, +1 = straight up.
			float pitchY = ctx.PlayerLookDir.y;
			if (pitchY > 0.707f)
			{
				props["facing"] = "up";
				return;
			}
			if (pitchY < -0.707f)
			{
				props["facing"] = "down";
				return;
			}
		}

		// --- Horizontal facing from player yaw ---
		// The block faces the player (opposite of look dir), matching Minecraft
		// convention for furnaces, pistons, etc.
		// We use the look direction's XZ components to determine the dominant axis.
		float x = ctx.PlayerLookDir.x;
		float z = ctx.PlayerLookDir.z;

		if (std::abs(x) >= std::abs(z))
		{
			// Dominant east/west axis
			props["facing"] = (x > 0.f) ? "west" : "east";
		}
		else
		{
			// Dominant north/south axis
			props["facing"] = (z > 0.f) ? "north" : "south";
		}
	}

	void BlockPlacementResolver::ResolveAxis(const PlacementContext& ctx, std::map<std::string, std::string>& props)
	{
		if (!BlockHasProperty(ctx.Id, "axis"))
			return;

		// The axis of a log/pillar aligns with the face that was clicked:
		//   Hit top or bottom face (y normal) → axis=y  (log stands vertically)
		//   Hit north or south face (z normal) → axis=z  (log lies along Z)
		//   Hit east or west face  (x normal) → axis=x  (log lies along X)
		if (ctx.HitFaceNormal.y != 0)
			props["axis"] = "y";
		else if (ctx.HitFaceNormal.z != 0)
			props["axis"] = "z";
		else
			props["axis"] = "x";
	}

	void BlockPlacementResolver::ResolveHalf(const PlacementContext& ctx, std::map<std::string, std::string>& props)
	{
		if (!BlockHasProperty(ctx.Id, "half"))
			return;

		// Hit top face → place in bottom half (sits on surface)
		if (ctx.HitFaceNormal.y == 1)
		{
			props["half"] = "bottom";
			return;
		}
		// Hit bottom face → place in top half (hangs from ceiling)
		if (ctx.HitFaceNormal.y == -1)
		{
			props["half"] = "top";
			return;
		}

		// Side face hit — use fractional Y of the hit position within the block:
		// upper half of the face (≥ 0.5) → top, lower half → bottom
		float fracY = ctx.HitPosition.y - std::floor(ctx.HitPosition.y);
		props["half"] = (fracY >= 0.5f) ? "top" : "bottom";
	}

	void BlockPlacementResolver::ResolveType(const PlacementContext& ctx, std::map<std::string, std::string>& props)
	{
		if (!BlockHasProperty(ctx.Id, "type"))
			return;

		// Hit top face → bottom slab (sits on surface)
		if (ctx.HitFaceNormal.y == 1)
		{
			props["type"] = "bottom";
			return;
		}
		// Hit bottom face → top slab (pressed against ceiling)
		if (ctx.HitFaceNormal.y == -1)
		{
			props["type"] = "top";
			return;
		}

		// Side face hit — use fractional Y of the hit position within the block:
		// upper half of the face (≥ 0.5) → top, lower half → bottom
		float fracY = ctx.HitPosition.y - std::floor(ctx.HitPosition.y);
		props["type"] = (fracY >= 0.5f) ? "top" : "bottom";
	}

	bool BlockPlacementResolver::BlockHasProperty(BlockId id, const std::string& propertyKey)
	{
		const auto& registry = BlockstateRegistry::Get();
		auto it = registry.find(id);
		if (it == registry.end())
			return false;

		for (const auto& variant : it->second)
		{
			if (variant.Properties.count(propertyKey) > 0)
				return true;
		}
		return false;
	}
} // namespace onion::voxel
