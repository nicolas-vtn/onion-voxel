#include "BlockPlacementResolver.hpp"

#include <shared/world/block/BlockstateRegistry.hpp>

namespace onion::voxel
{
	// -------------------------------------------------------------------------
	// Public
	// -------------------------------------------------------------------------

	PlacementResult BlockPlacementResolver::Resolve(const PlacementContext& ctx)
	{
		PlacementResult result;
		result.Position = ctx.PlacePosition;
		result.Id = ctx.Id;

		ResolveDirectionalFacing(ctx, result);
		ResolveAxis(ctx, result);
		ResolveHalf(ctx, result);
		ResolveType(ctx, result); // last — may redirect Position and Id
		ResolveOrientation(ctx, result);
		ResolveFace(ctx, result);

		return result;
	}

	// -------------------------------------------------------------------------
	// Private
	// -------------------------------------------------------------------------

	void BlockPlacementResolver::ResolveDirectionalFacing(const PlacementContext& ctx, PlacementResult& result)
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
				result.Properties["facing"] = "down";
				return;
			}
			if (ctx.HitFaceNormal.y == -1)
			{
				result.Properties["facing"] = "up";
				return;
			}

			// Also check player pitch for steep looks when placing on side faces.
			// If looking steeply up/down (pitch > 45°), prefer vertical.
			// PlayerLookDir.y: -1 = straight down, +1 = straight up.
			float pitchY = ctx.PlayerLookDir.y;
			if (pitchY > 0.707f)
			{
				result.Properties["facing"] = "up";
				return;
			}
			if (pitchY < -0.707f)
			{
				result.Properties["facing"] = "down";
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
			result.Properties["facing"] = (x > 0.f) ? "west" : "east";
		else
			result.Properties["facing"] = (z > 0.f) ? "north" : "south";
	}

	void BlockPlacementResolver::ResolveAxis(const PlacementContext& ctx, PlacementResult& result)
	{
		if (!BlockHasProperty(ctx.Id, "axis"))
			return;

		// The axis of a log/pillar aligns with the face that was clicked:
		//   Hit top or bottom face (y normal) → axis=y  (log stands vertically)
		//   Hit north or south face (z normal) → axis=z  (log lies along Z)
		//   Hit east or west face  (x normal) → axis=x  (log lies along X)
		if (ctx.HitFaceNormal.y != 0)
			result.Properties["axis"] = "y";
		else if (ctx.HitFaceNormal.z != 0)
			result.Properties["axis"] = "z";
		else
			result.Properties["axis"] = "x";
	}

	void BlockPlacementResolver::ResolveHalf(const PlacementContext& ctx, PlacementResult& result)
	{
		if (!BlockHasProperty(ctx.Id, "half"))
			return;

		// Hit top face → place in bottom half (sits on surface)
		if (ctx.HitFaceNormal.y == 1)
		{
			result.Properties["half"] = "bottom";
			return;
		}
		// Hit bottom face → place in top half (hangs from ceiling)
		if (ctx.HitFaceNormal.y == -1)
		{
			result.Properties["half"] = "top";
			return;
		}

		// Side face hit — use fractional Y of the hit position within the block:
		// upper half of the face (≥ 0.5) → top, lower half → bottom
		float fracY = ctx.HitPosition.y - std::floor(ctx.HitPosition.y);
		result.Properties["half"] = (fracY >= 0.5f) ? "top" : "bottom";
	}

	void BlockPlacementResolver::ResolveType(const PlacementContext& ctx, PlacementResult& result)
	{
		if (!BlockHasProperty(ctx.Id, "type"))
			return;

		// --- Promotion: placing a slab onto the hit slab directly (hit block is same type) ---
		if (ctx.World != nullptr && ctx.HitBlock.ID() == ctx.Id)
		{
			const auto& registry = BlockstateRegistry::Get();
			auto it = registry.find(ctx.HitBlock.ID());
			if (it != registry.end())
			{
				const auto& hitVariant = it->second[ctx.HitBlock.State.VariantIndex];
				auto typeIt = hitVariant.Properties.find("type");
				if (typeIt != hitVariant.Properties.end())
				{
					const std::string& existingType = typeIt->second;

					// bottom slab hit on top face → promote to double
					if (existingType == "bottom" && ctx.HitFaceNormal.y == 1)
					{
						result.Position = ctx.HitBlock.Position;
						result.Properties["type"] = "double";
						result.IsPromotion = true;
						return;
					}

					// top slab hit on bottom face → promote to double
					if (existingType == "top" && ctx.HitFaceNormal.y == -1)
					{
						result.Position = ctx.HitBlock.Position;
						result.Properties["type"] = "double";
						result.IsPromotion = true;
						return;
					}
				}
			}
		}

		// --- Promotion: PlacePosition already contains a slab of the same BlockId ---
		// Resolve what type would normally be placed, then check for a complement.
		// Complementary types (bottom+top) promote to double.
		// Same type falls through — blocked by the air-only guard in the caller.
		if (ctx.World != nullptr)
		{
			BlockState existingState = ctx.World->GetBlock(ctx.PlacePosition);
			if (existingState.ID == ctx.Id)
			{
				const auto& registry = BlockstateRegistry::Get();
				auto it = registry.find(existingState.ID);
				if (it != registry.end())
				{
					const auto& existingVariant = it->second[existingState.VariantIndex];
					auto typeIt = existingVariant.Properties.find("type");
					if (typeIt != existingVariant.Properties.end())
					{
						const std::string& existingType = typeIt->second;

						std::string resolvedType;
						if (ctx.HitFaceNormal.y == 1)
							resolvedType = "bottom";
						else if (ctx.HitFaceNormal.y == -1)
							resolvedType = "top";
						else
						{
							float fracY = ctx.HitPosition.y - std::floor(ctx.HitPosition.y);
							resolvedType = (fracY >= 0.5f) ? "top" : "bottom";
						}

						if ((existingType == "bottom" && resolvedType == "top") ||
							(existingType == "top" && resolvedType == "bottom"))
						{
							result.Position = ctx.PlacePosition;
							result.Properties["type"] = "double";
							result.IsPromotion = true;
							return;
						}

						// Same type or double → fall through; air-only guard will block
					}
				}
			}
		}

		// --- Normal resolution ---
		if (ctx.HitFaceNormal.y == 1)
		{
			result.Properties["type"] = "bottom";
			return;
		}
		else if (ctx.HitFaceNormal.y == -1)
		{
			result.Properties["type"] = "top";
			return;
		}

		float fracY = ctx.HitPosition.y - std::floor(ctx.HitPosition.y);
		result.Properties["type"] = (fracY >= 0.5f) ? "top" : "bottom";
	}

	// Returns the opposite cardinal horizontal direction.
	static std::string Opposite(const std::string& dir)
	{
		if (dir == "north")
			return "south";
		if (dir == "south")
			return "north";
		if (dir == "east")
			return "west";
		if (dir == "west")
			return "east";
		return dir;
	}

	// Returns the dominant horizontal direction from an XZ look vector.
	static std::string DominantHorizontal(float x, float z)
	{
		if (std::abs(x) >= std::abs(z))
			return (x > 0.f) ? "west" : "east";
		else
			return (z > 0.f) ? "north" : "south";
	}

	void BlockPlacementResolver::ResolveOrientation(const PlacementContext& ctx, PlacementResult& result)
	{
		if (!BlockHasProperty(ctx.Id, "orientation"))
			return;

		const float x = ctx.PlayerLookDir.x;
		const float y = ctx.PlayerLookDir.y;
		const float z = ctx.PlayerLookDir.z;

		// Step 1: pointing direction from camera look
		std::string pointing;
		if (y < -0.707f)
			pointing = "down";
		else if (y > 0.707f)
			pointing = "up";
		else
			pointing = DominantHorizontal(x, z);

		// Step 2: top direction
		std::string top;
		if (pointing == "north" || pointing == "south" || pointing == "east" || pointing == "west")
		{
			// Horizontal pointing — block sits upright naturally
			top = "up";
		}
		else
		{
			// Vertical pointing — top follows horizontal look direction
			const std::string lookH = DominantHorizontal(x, z);
			top = (pointing == "down") ? lookH : Opposite(lookH);
		}

		result.Properties["orientation"] = pointing + "_" + top;
	}

	void BlockPlacementResolver::ResolveFace(const PlacementContext& ctx, PlacementResult& result)
	{
		if (!BlockHasProperty(ctx.Id, "face"))
			return;

		const float x = ctx.PlayerLookDir.x;
		const float z = ctx.PlayerLookDir.z;

		if (ctx.HitFaceNormal.y == 1)
		{
			// Hit the top face of a block → button sits on the floor, facing player look direction.
			result.Properties["face"] = "floor";
			result.Properties["facing"] = DominantHorizontal(x, z);
		}
		else if (ctx.HitFaceNormal.y == -1)
		{
			// Hit the bottom face of a block → button hangs from ceiling, facing player look direction.
			result.Properties["face"] = "ceiling";
			result.Properties["facing"] = DominantHorizontal(x, z);
		}
		else
		{
			// Hit a side face → button mounts on wall, facing outward (away from the wall surface).
			result.Properties["face"] = "wall";
			if (ctx.HitFaceNormal.z == 1)
				result.Properties["facing"] = "south";
			else if (ctx.HitFaceNormal.z == -1)
				result.Properties["facing"] = "north";
			else if (ctx.HitFaceNormal.x == 1)
				result.Properties["facing"] = "east";
			else
				result.Properties["facing"] = "west";
		}
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
