#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "miniz.h"

namespace onion::voxel
{
	class ZipArchive
	{
		// ------------ Constructor / Destructor ------------
	  public:
		explicit ZipArchive(const std::filesystem::path& filePath);
		~ZipArchive();

		ZipArchive(const ZipArchive&) = delete;
		ZipArchive& operator=(const ZipArchive&) = delete;

		ZipArchive(ZipArchive&&) noexcept;
		ZipArchive& operator=(ZipArchive&&) noexcept;

		// ------------ Public API ------------
	  public:
		std::vector<std::string> GetFileList(const std::filesystem::path& directory = "") const;

		bool FileExists(const std::filesystem::path& filePath) const;

		std::string GetFileText(const std::filesystem::path& filePath) const;

		std::vector<unsigned char> GetFileBinary(const std::filesystem::path& filePath) const;

		// ------------ Private Members ------------
	  private:
		mutable mz_zip_archive m_Zip{};
		bool m_IsOpen = false;
	};

} // namespace onion::voxel
