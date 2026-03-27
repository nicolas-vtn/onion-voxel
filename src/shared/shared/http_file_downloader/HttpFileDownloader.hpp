#pragma once

namespace onion::voxel
{
	class HttpFileDownloader
	{
	  public:
		static bool DownloadFile(const std::string& url, const std::filesystem::path& destinationPath) { return true; }
	};
} // namespace onion::voxel
