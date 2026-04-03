#pragma once

#include "resource_pack_manager/ResourcePackManager.hpp"

namespace onion::voxel
{
	class AssetsManager
	{
		// ----- Constructor / Destructor -----
	  public:
		AssetsManager();
		~AssetsManager();

		// ----- Public API -----
	  public:
		/// @brief Gets the text content of a file at the specified path. The path is relative to the assets directory.
		/// @param path The relative path to the file within the assets directory.
		/// @return The text content of the file.
		std::string GetFileText(const std::filesystem::path& path) const;

		/// @brief Gets the binary content of a file at the specified path. The path is relative to the assets directory.
		/// @param path The relative path to the file within the assets directory.
		/// @return A vector containing the binary content of the file.
		std::vector<unsigned char> GetFileBinary(const std::filesystem::path& path) const;

		/// @brief Gets the text content of a file from the currently selected resource pack. The path is relative to the root of the resource pack.
		/// @param path The relative path to the file within the resource pack.
		/// @return The text content of the file.
		std::string GetResourcePackFileText(const std::filesystem::path& path) const;
		/// @brief Gets the binary content of a file from the currently selected resource pack. The path is relative to the root of the resource pack.
		/// @param path The relative path to the file within the resource pack.
		/// @return A vector containing the binary content of the file.
		std::vector<unsigned char> GetResourcePackFileBinary(const std::filesystem::path& path) const;

		/// @brief Sets the current resource pack name to use for asset retrieval
		/// @param resourcePack The name of the resource pack to set as current.
		void SetCurrentResourcePack(const std::string& resourcePack);
		/// @brief Gets the name of the currently selected resource pack
		/// @return The name of the currently selected resource pack.
		std::string GetCurrentResourcePack() const;

		static std::filesystem::path GetAssetsDirectory();
		static std::filesystem::path GetTexturesDirectory();
		static std::filesystem::path GetShadersDirectory();
		static std::filesystem::path GetResourcePacksDirectory();
		static std::filesystem::path GetTextsDirectory();
		static std::filesystem::path GetAppIconsDirectory();

		// ----- Private Methods / Members -----
	  private:
		static std::filesystem::path s_ExecutableDirectory;

		static inline const std::string ASSETS_FOLDER_NAME = "assets";
		static inline const std::string TEXTURES_FOLDER_NAME = "textures";
		static inline const std::string SHADERS_FOLDER_NAME = "shaders";
		static inline const std::string RESOURCE_PACKS_FOLDER_NAME = "resourcepacks";
		static inline const std::string TEXTS_FOLDER_NAME = "texts";
		static inline const std::string APPICONS_FOLDER_NAME = "app_icons";

		ResourcePackManager m_ResourcePackManager;
	};
} // namespace onion::voxel
