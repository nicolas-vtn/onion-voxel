#pragma once

#include <filesystem>

#include "controls/ControlsSettings.hpp"
#include "quality_and_perf_settings/QualityAndPerfSettings.hpp"
#include "video_settings/VideoSettings.hpp"

namespace onion::voxel
{
	class UserSettings
	{
		// ----- Constructor / Destructor -----
	  public:
		UserSettings() = default;
		~UserSettings() = default;

		// ----- Public Members -----
	  public:
		std::string Version = "1.0";
		ControlsSettings Controls{};
		QualityAndPerfSettings QualityAndPerf{};
		VideoSettings Video{};

		// ----- Load / Save -----
	  public:
		static UserSettings Load(const std::filesystem::path& filePath);
		static void Save(const UserSettings& settings, const std::filesystem::path& filePath);
	};

	template <typename BasicJsonType> inline void to_json(BasicJsonType& j, const UserSettings& s)
	{
		j = BasicJsonType{
			{"Version", s.Version}, {"QualityAndPerf", s.QualityAndPerf}, {"Video", s.Video}, {"Controls", s.Controls}};
	}

	template <typename BasicJsonType> inline void from_json(const BasicJsonType& j, UserSettings& s)
	{
		if (j.contains("Version"))
			j.at("Version").get_to(s.Version);

		if (j.contains("Controls"))
			j.at("Controls").get_to(s.Controls);

		if (j.contains("QualityAndPerf"))
			j.at("QualityAndPerf").get_to(s.QualityAndPerf);

		if (j.contains("Video"))
			j.at("Video").get_to(s.Video);
	}

} // namespace onion::voxel
