#pragma once

#include <glm/glm.hpp>

#include <vector>

#include "BlockIds.hpp"

namespace onion::voxel
{
	class BlockState
	{
		// ----- Constructor / Destructor -----
	  public:
		BlockState() = default;
		explicit BlockState(BlockId blockID) : ID(blockID) {}
		~BlockState() = default;

		// ----- Operators -----
	  public:
		bool operator==(const BlockState& other) const;
		bool operator!=(const BlockState& other) const;

		// ----- Members -----
	  public:
		BlockId ID = BlockId::Air;

		// Index into BlockRegistry's per-block variant list.
		// 0 = default / first variant. Resolved at block placement or chunk load.
		uint8_t VariantIndex = 0;

		// ----- Static Helpers -----
	  public:
		static bool IsOpaque(BlockId blockID);
		static bool IsTransparent(BlockId blockID);
		static bool IsSolid(BlockId blockID);

		// ----- Static Members -----
	  private:
		// A lookup table for block transparency, indexed by BlockId
		static const std::vector<bool> s_TransparencyLookupTable;

		// A lookup table for block solidity, indexed by BlockId
		static const std::vector<bool> s_SolidLookupTable;

		// ----- Lists of block IDs for different categories -----
	  public:
		static const std::vector<BlockId> Flowers;
	};
} // namespace onion::voxel
