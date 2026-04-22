#include "ZipArchive.hpp"

#include <cstring>
#include <stdexcept>

namespace onion::voxel
{

	ZipArchive::ZipArchive(const std::filesystem::path& filePath)
	{
		memset(&m_Zip, 0, sizeof(m_Zip));

		if (!mz_zip_reader_init_file(&m_Zip, filePath.string().c_str(), 0))
		{
			throw std::runtime_error("Failed to open zip archive: " + filePath.string());
		}

		m_IsOpen = true;
	}

	ZipArchive::~ZipArchive()
	{
		if (m_IsOpen)
		{
			mz_zip_reader_end(&m_Zip);
		}
	}

	ZipArchive::ZipArchive(ZipArchive&& other) noexcept
	{
		m_Zip = other.m_Zip;
		m_IsOpen = other.m_IsOpen;

		other.m_IsOpen = false;
	}

	ZipArchive& ZipArchive::operator=(ZipArchive&& other) noexcept
	{
		if (this != &other)
		{
			if (m_IsOpen)
				mz_zip_reader_end(&m_Zip);

			m_Zip = other.m_Zip;
			m_IsOpen = other.m_IsOpen;

			other.m_IsOpen = false;
		}

		return *this;
	}

	bool ZipArchive::FileExists(const std::filesystem::path& filePath) const
	{
		return mz_zip_reader_locate_file(&m_Zip, filePath.generic_string().c_str(), nullptr, 0) >= 0;
	}

	std::vector<std::filesystem::path> ZipArchive::GetFileList(const std::filesystem::path& directory) const
	{
		std::vector<std::filesystem::path> files;

		mz_uint fileCount = mz_zip_reader_get_num_files(&m_Zip);

		std::string dir = directory.generic_string();
		if (!dir.empty() && dir.back() != '/')
			dir += '/';

		for (mz_uint i = 0; i < fileCount; i++)
		{
			mz_zip_archive_file_stat stat;

			if (!mz_zip_reader_file_stat(&m_Zip, i, &stat))
				continue;

			std::string name = stat.m_filename;

			if (!dir.empty())
			{
				if (name.rfind(dir, 0) != 0)
					continue;
			}

			files.emplace_back(name);
		}

		return files;
	}

	std::string ZipArchive::GetFileText(const std::filesystem::path& filePath) const
	{
		size_t size = 0;

		void* data = mz_zip_reader_extract_file_to_heap(&m_Zip, filePath.generic_string().c_str(), &size, 0);

		if (!data)
			return {};

		std::string result(static_cast<char*>(data), size);

		mz_free(data);

		return result;
	}

	std::vector<unsigned char> ZipArchive::GetFileBinary(const std::filesystem::path& filePath) const
	{
		size_t size = 0;

		void* data = mz_zip_reader_extract_file_to_heap(&m_Zip, filePath.generic_string().c_str(), &size, 0);

		if (!data)
			return {};

		std::vector<unsigned char> buffer(size);
		std::memcpy(buffer.data(), data, size);

		mz_free(data);

		return buffer;
	}

} // namespace onion::voxel
