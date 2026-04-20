#pragma once

#include <filesystem>
#include <string>

namespace onion::voxel
{
	class BlockModel
	{
		// ----- Enums -----
	  public:
		enum class eParent
		{
			CubeAll
		};

		class Textures
		{
		  public:
			std::string All;
		};

		// ----- Constructor / Destructor -----
	  public:
		BlockModel() = default;
		~BlockModel() = default;

		// ----- Public API -----
	  public:
		static BlockModel FromFile(const std::filesystem::path& modelPath);

		// ----- Members -----
	  public:
		eParent Parent = eParent::CubeAll;
		Textures ModelTextures;
	};
} // namespace onion::voxel
