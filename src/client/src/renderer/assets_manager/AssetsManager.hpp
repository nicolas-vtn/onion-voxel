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
		std::string GetFileText(const std::filesystem::path& path) const;
		std::vector<unsigned char> GetFileBinary(const std::filesystem::path& path) const;

		std::string GetResourcePackFileText(const std::filesystem::path& path) const;
		std::vector<unsigned char> GetResourcePackFileBinary(const std::filesystem::path& path) const;

		void SetCurrentResourcePack(const std::string& resourcePack);
		std::string GetCurrentResourcePack() const;

		std::filesystem::path GetAssetsDirectory() const;
		std::filesystem::path GetResourcePacksDirectory() const;

		// ----- Private Methods / Members -----
	  private:
		std::filesystem::path m_AssetsDirectory;
		ResourcePackManager m_ResourcePackManager;
	};
} // namespace onion::voxel
