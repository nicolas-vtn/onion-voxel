#pragma once

#include <array>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include <shared/zip_archive/ZipArchive.hpp>

namespace onion::voxel
{
	class BlockModel
	{
		// ----- Types -----
	  public:
		// Maps texture alias name (without '#') to its value, e.g. "side" -> "block/oak_log"
		using Textures = std::unordered_map<std::string, std::string>;

		struct Face
		{
			std::array<uint8_t, 4> UV{0, 0, 16, 16};
			std::string Texture; // "#side" or resolved later
			std::optional<std::string> CullFace;
			std::optional<int> TintIndex;
		};

		struct Element
		{
			std::array<uint8_t, 3> From{0, 0, 0};
			std::array<uint8_t, 3> To{0, 0, 0};

			std::unordered_map<std::string, Face> Faces;
			// keys: "north", "south", etc.
		};

		// ----- Constructor / Destructor -----
	  public:
		BlockModel() = default;
		~BlockModel() = default;

		// ----- Public API -----
	  public:
		static void SetModelArchive(const std::filesystem::path& archiveFilePath);
		static BlockModel FromFile(const std::string& filename);
		static void ClearCache();

		// ----- Members -----
	  public:
		Textures ModelTextures;
		std::vector<Element> Elements;

		// ----- Private Members -----
	  private:
		std::string ParentPath; // raw parent path from JSON, resolved to actual model later

		// ----- Private Methods -----
	  private:
		static std::filesystem::path ResolveModelPath(const std::string& parentPath);
		static void MergeModels(BlockModel& parent, const BlockModel& child);
		static BlockModel LoadRawModel(const std::filesystem::path& modelPath);
		static BlockModel LoadModelRecursive(const std::filesystem::path& modelPath);

		// ----- Static Private Members -----
	  private:
		static inline std::unique_ptr<ZipArchive> s_ModelArchive;
		static inline std::unordered_map<std::filesystem::path, BlockModel> s_ModelCache;
		static inline std::recursive_mutex s_CacheMutex;
		static const BlockModel& GetModel(const std::filesystem::path& path);
	};
} // namespace onion::voxel
