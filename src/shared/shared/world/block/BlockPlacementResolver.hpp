#pragma once

#include <map>
#include <string>

#include <glm/glm.hpp>

#include <shared/world/block/Block.hpp>
#include <shared/world/block/BlockId.hpp>
#include <shared/world/world_manager/WorldManager.hpp>

namespace onion::voxel
{
	/// Context provided at block placement time.
	struct PlacementContext
	{
		/// The block type being placed.
		BlockId Id = BlockId::Air;

		/// Normalized camera forward vector (player look direction).
		glm::vec3 PlayerLookDir{0.f};

		/// Face normal of the hit block face (points from hit block toward adjacent/place position).
		/// E.g. (0,1,0) = top face was hit, block will be placed above.
		glm::ivec3 HitFaceNormal{0};

		/// World position where the block will be placed.
		glm::ivec3 PlacePosition{0};

		/// Exact world-space position where the ray hit the block surface.
		/// Used for half= / type= resolution on side face hits.
		glm::vec3 HitPosition{0.f};

		/// The block that was hit by the raycast.
		/// Used by promotion resolvers (e.g. slab stacking) to inspect the existing block.
		Block HitBlock{};

		/// World manager for neighbor queries.
		const WorldManager* World = nullptr;
	};

	/// The resolved placement intent returned by BlockPlacementResolver::Resolve.
	/// Resolvers may redirect both the position and the block ID (e.g. slab promotion).
	struct PlacementResult
	{
		/// Where the block will be placed. Defaults to ctx.PlacePosition.
		/// Promotion resolvers may redirect this to ctx.HitBlock.Position.
		glm::ivec3 Position{0};

		/// The block ID to place. Defaults to ctx.Id.
		/// Promotion resolvers may change this (e.g. potted plant variants).
		BlockId Id;

		/// Resolved blockstate properties. Pass to BlockstateRegistry::GetVariantIndex.
		std::map<std::string, std::string> Properties;

		/// True when the placement intentionally overwrites an existing non-air block
		/// (e.g. slab promotion to double). Used by the caller to bypass the air-only guard.
		bool IsPromotion = false;
	};

	/// Resolves the correct blockstate properties for a block being placed.
	///
	/// The resolver is data-driven: it inspects the available variant properties
	/// in the BlockstateRegistry to decide which properties to set, rather than
	/// relying on hard-coded block lists.
	///
	/// Currently supports:
	///   - Directional facing (facing=north/south/east/west/up/down)
	///   - Axis-aligned blocks (logs, pillars — axis=x/y/z)
	///   - Vertical half (stairs, trapdoors, doors — half=bottom/top)
	///   - Slab type (slabs — type=bottom/top/double via promotion)
	///
	/// Designed for extension: add more Resolve* private methods for:
	///   - Wall-mounted blocks (torches, buttons, levers)
	///   - Connected blocks (fences, walls, panes)
	///   - Promotion (flowers into pots, etc.)
	class BlockPlacementResolver
	{
	  public:
		/// Compute the full placement intent for the block described by ctx.
		static PlacementResult Resolve(const PlacementContext& ctx);

	  private:
		/// Populate facing= based on player look direction / hit face.
		static void ResolveDirectionalFacing(const PlacementContext& ctx, PlacementResult& result);

		/// Populate axis= (x/y/z) based on the hit face normal — for logs, pillars, etc.
		static void ResolveAxis(const PlacementContext& ctx, PlacementResult& result);

		/// Populate half= (bottom/top) based on the hit face normal and hit position Y fraction.
		static void ResolveHalf(const PlacementContext& ctx, PlacementResult& result);

		/// Populate type= (bottom/top) based on hit face / Y fraction.
		/// Promotes to type=double when placing a matching slab onto an existing one.
		static void ResolveType(const PlacementContext& ctx, PlacementResult& result);

		/// Returns true if any variant of the given block has the specified property key.
		static bool BlockHasProperty(BlockId id, const std::string& propertyKey);
	};
} // namespace onion::voxel
