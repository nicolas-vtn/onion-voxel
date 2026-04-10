#pragma once

#include <onion/Event.hpp>

#include <shared/zip_archive/ZipArchive.hpp>

#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace onion::voxel
{
	class ResourcePackManager
	{
		// ----- Constructor & Destructor -----
	  public:
		ResourcePackManager();
		~ResourcePackManager();

		// ----- Public API -----
	  public:
		std::string GetFileText(const std::filesystem::path& path) const;
		std::vector<unsigned char> GetFileBinary(const std::filesystem::path& path) const;

		void SetResourcePackDirectory(const std::filesystem::path& directory);

		// ----- Events -----
	  public:
		Event<const std::string&> EvtResourcePackChanged;

		// ----- Getters / Setters -----
	  public:
		std::string GetCurrentResourcePack() const;
		void SetCurrentResourcePack(const std::string& resourcePack);

		// ----- Private Methods -----
	  private:
		// ----- Static Members -----
	  private:
		static inline const std::string DEFAULT_RESOURCE_PACK = "Default";

		// ----- Private Members -----
	  private:
		mutable std::mutex m_Mutex;
		std::string m_CurrentResourcePack = DEFAULT_RESOURCE_PACK;
		std::filesystem::path m_ResourcePackDirectory;

		std::unique_ptr<ZipArchive> m_ResourcePackArchive;
		std::unique_ptr<ZipArchive> m_DefaultResourcePackArchive;
	};
} // namespace onion::voxel
