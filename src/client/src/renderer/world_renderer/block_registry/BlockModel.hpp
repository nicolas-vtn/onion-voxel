#pragma once

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>

namespace onion::voxel
{
	class BlockModel
	{
		// ----- Enums -----
	  public:
		enum class eParent
		{
			CubeAll,
			Block
		};

		class Textures
		{
		  public:
			std::string All;
			std::string Particle;
			std::string Bottom;
			std::string Top;
			std::string Side;
			std::string Overlay;
		};

		struct Face
		{
			std::array<float, 4> UV{0, 0, 0, 0};
			std::string Texture; // "#side" or resolved later
			std::optional<std::string> CullFace;
			std::optional<int> TintIndex;
		};

		struct Element
		{
			std::array<float, 3> From{0, 0, 0};
			std::array<float, 3> To{0, 0, 0};

			std::unordered_map<std::string, Face> Faces;
			// keys: "north", "south", etc.
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
		std::vector<Element> Elements;
	};
} // namespace onion::voxel
