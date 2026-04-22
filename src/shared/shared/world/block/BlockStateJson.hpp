#pragma once

#include <map>
#include <string>
#include <vector>

namespace onion::voxel
{
	// A single model entry within a blockstate variant (weighted random models use an array)
	struct VariantModel
	{
		std::string ModelPath; // e.g. "block/acacia_log_horizontal"
		int RotationX = 0;	   // 0 / 90 / 180 / 270
		int RotationY = 0;	   // 0 / 90 / 180 / 270
		bool UvLock = false;
		int Weight = 1;
	};

	// One variant entry: a set of property conditions → one or more weighted models
	struct BlockStateVariant
	{
		// Parsed conditions from the key string, e.g. "axis=x,waterlogged=false"
		// → { "axis" -> "x", "waterlogged" -> "false" }
		// An empty map means the variant matches all states (key was "").
		std::map<std::string, std::string> Conditions;

		// Usually one model; multiple entries = weighted random selection
		std::vector<VariantModel> Models;
	};

	class BlockStateJson
	{
		// ----- Constructor / Destructor -----
	  public:
		BlockStateJson() = default;
		~BlockStateJson() = default;

		// ----- Public API -----
	  public:
		static BlockStateJson FromJson(const std::string& jsonText);

		// Returns true if this blockstate uses the "variants" format
		bool HasVariants() const { return !Variants.empty(); }

		// ----- Members -----
	  public:
		// All parsed variants (variants format). Order matches the JSON iteration order.
		std::vector<BlockStateVariant> Variants;
	};
} // namespace onion::voxel
