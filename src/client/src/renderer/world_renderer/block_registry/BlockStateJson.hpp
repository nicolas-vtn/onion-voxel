#pragma once

#include <filesystem>
#include <string>

namespace onion::voxel
{
	class BlockStateJson
	{
		// ----- Constructor / Destructor -----
	  public:
		BlockStateJson() = default;
		~BlockStateJson() = default;

		// ----- Public API -----
	  public:
		static BlockStateJson FromFile(const std::string& filename);

		// ----- Members -----
	  public:
		std::string ModelPath; // raw model path from JSON, resolved to actual model later
	};
} // namespace onion::voxel
