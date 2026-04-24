#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include <shared/utils/Utils.hpp>
#include <shared/world/block/BlockIds.hpp>

#include "BlockModel.hpp"

namespace onion::voxel
{
	struct VariantModel
	{
		std::map<std::string, std::string> Properties;
		BlockModel Model;
		int RotationX = 0;
		int RotationY = 0;
		bool UVLock = false;
	};

	class BlockstateRegistry
	{
		// ----- Public API -----
	  public:
		static const std::unordered_map<BlockId, std::vector<VariantModel>>& Get();

		static bool IsTallPlant(BlockId flowerId);
		static uint8_t GetVariantIndex(BlockId id, const std::map<std::string, std::string>& properties);

		static bool CountsInAO(BlockId id, uint8_t variantIndex);

		// ----- Private Methods -----
	  private:
		static inline const std::filesystem::path s_BlockstateArchiveFilePath =
			Utils::GetExecutableDirectory() / "assets" / "blockstates.zip";
		static inline const std::filesystem::path s_ModelArchiveFilePath =
			Utils::GetExecutableDirectory() / "assets" / "models.zip";

		static std::unordered_map<BlockId, std::vector<VariantModel>> LoadVariantsModel();

		static VariantModel VariantModelFromJson(const nlohmann::json& json);
	};
} // namespace onion::voxel
