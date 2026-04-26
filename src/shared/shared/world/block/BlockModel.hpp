#pragma once

#include <array>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include <glm/glm.hpp>

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
			std::array<float, 4> UV{0, 0, 16, 16};
			std::string Texture; // "#side" or resolved later
			std::optional<std::string> CullFace;
			std::optional<int> TintIndex;
			std::optional<float> Rotation;
		};

		struct ElementRotation
		{
			glm::vec3 Origin{0, 0, 0};
			std::string Axis; // "x", "y", or "z"
			float Angle{0.0f};
			bool Rescale = false;
		};

		struct Element
		{
			glm::vec3 From{0, 0, 0};
			glm::vec3 To{0, 0, 0};
			ElementRotation Rotation;
			bool Shade = true;

			std::unordered_map<std::string, Face> Faces;
			// keys: "north", "south", etc.
		};

		struct DisplayInfo
		{
			glm::vec3 Rotation{0, 0, 0};
			glm::vec3 Translation{0, 0, 0};
			glm::vec3 Scale{1, 1, 1};
		};

		struct Display
		{
			DisplayInfo Gui;
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
		bool AmbientOcclusion = true;
		Textures ModelTextures;
		std::vector<Element> Elements;
		Display ModelDisplay;

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
