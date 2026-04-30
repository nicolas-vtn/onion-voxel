#pragma once

#include <glm/glm.hpp>

#include <shared_mutex>
#include <unordered_map>
#include <vector>

#include "BlockIds.hpp"

namespace onion::voxel
{
	class BlockState
	{

		// ----- Constructor / Destructor -----
	  public:
		BlockState() = default;
		explicit BlockState(BlockId blockID);
		explicit BlockState(BlockId blockID, uint8_t variantIndex);
		~BlockState() = default;

		// ----- Operators -----
	  public:
		bool operator==(const BlockState& other) const;
		bool operator!=(const BlockState& other) const;

		// ----- Members -----
	  public:
		BlockId ID = BlockId::Air; // The block ID (type) of this block

		uint8_t VariantIndex = 0; // The variant index for this block (cf BlockstateRegistry::Get())

		// ----- Static Helpers -----
	  public:
		static bool IsTransparent(BlockId blockID);
		static bool IsSolid(BlockId blockID);
		static bool IsFlower(BlockId blockId);

		/// Returns true if the given block variant completely fills its voxel cell
		static bool IsFullBlock(BlockId blockID, uint8_t variantIndex);

		static void SetTransparency(BlockId blockID, bool transparent);

		/// Builds the IsFullBlock lookup table from the BlockstateRegistry.
		/// Must be called once after the registry is fully loaded (after ReloadTextures).
		static void BuildFullBlockLookup();

		// ----- Static Members -----
	  private:
		// A lookup table for block transparency, indexed by BlockId
		static std::vector<bool> s_TransparencyLookupTable;
		static inline std::shared_mutex s_TransparencyLookupTableMutex;

		// A lookup table for block solidity, indexed by BlockId
		static const std::vector<bool> s_SolidLookupTable;

		// Per-variant full-block lookup: [BlockId] -> [variantIndex] -> bool
		// Write-once at load time; no mutex required.
		static std::unordered_map<BlockId, std::vector<bool>> s_FullBlockLookupTable;

		// ----- Lists of block IDs for different categories -----
	  public:
		static const std::vector<BlockId> Flowers;
	};
} // namespace onion::voxel
