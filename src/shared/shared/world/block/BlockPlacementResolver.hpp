#pragma once

#include <map>
#include <string>

#include <glm/glm.hpp>

#include <shared/world/block/BlockId.hpp>
#include <shared/world/world_manager/WorldManager.hpp>

namespace onion::voxel
{
	/// Context provided at block placement time.
	struct PlacementContext
	{
		/// The block type being placed.
		BlockId Id;

		/// Normalized camera forward vector (player look direction).
		glm::vec3 PlayerLookDir{0.f};

		/// Face normal of the hit block face (points from hit block toward adjacent/place position).
		/// E.g. (0,1,0) = top face was hit, block will be placed above.
		glm::ivec3 HitFaceNormal{0};

		/// World position where the block will be placed.
		glm::ivec3 PlacePosition{0};

		/// World manager for neighbor queries (may be nullptr — unused for now).
		const WorldManager* World = nullptr;
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
	///
	/// Designed for extension: add more Resolve* private methods for:
	///   - Slabs/stairs (half)
	///   - Wall-mounted blocks (torches, buttons, levers)
	///   - Connected blocks (fences, walls, panes)
	class BlockPlacementResolver
	{
	  public:
		/// Compute the property map for the block described by ctx.
		/// Pass the result directly to BlockstateRegistry::GetVariantIndex.
		static std::map<std::string, std::string> Resolve(const PlacementContext& ctx);

	  private:
		/// Populate facing= based on player look direction / hit face.
		static void ResolveDirectionalFacing(const PlacementContext& ctx, std::map<std::string, std::string>& props);

		/// Populate axis= (x/y/z) based on the hit face normal — for logs, pillars, etc.
		static void ResolveAxis(const PlacementContext& ctx, std::map<std::string, std::string>& props);

		/// Returns true if any variant of the given block has the specified property key.
		static bool BlockHasProperty(BlockId id, const std::string& propertyKey);
	};
} // namespace onion::voxel
