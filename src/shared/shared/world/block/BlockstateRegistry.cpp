#include "BlockstateRegistry.hpp"

#include <shared/zip_archive/ZipArchive.hpp>

namespace
{
	static std::string ToBlockName(const std::string& blockstateName)
	{
		std::string name = blockstateName;

		// ---- Remove ".json" extension ----
		const std::string ext = ".json";
		if (name.size() >= ext.size() && name.substr(name.size() - ext.size()) == ext)
		{
			name.erase(name.size() - ext.size());
		}

		// ---- Replace '_' with ' ' ----
		std::replace(name.begin(), name.end(), '_', ' ');

		// ---- Capitalize each word ----
		bool capitalizeNext = true;
		for (char& c : name)
		{
			if (std::isspace(static_cast<unsigned char>(c)))
			{
				capitalizeNext = true;
			}
			else if (capitalizeNext)
			{
				c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
				capitalizeNext = false;
			}
			else
			{
				c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
			}
		}

		return name;
	}

	// Parse "axis=x,waterlogged=false" → { "axis" -> "x", "waterlogged" -> "false" }
	// An empty key string → empty map (matches-all variant)
	static std::map<std::string, std::string> ParseConditions(const std::string& key)
	{
		std::map<std::string, std::string> conditions;
		if (key.empty())
			return conditions;

		std::istringstream stream(key);
		std::string token;
		while (std::getline(stream, token, ','))
		{
			const size_t eq = token.find('=');
			if (eq == std::string::npos)
				continue;
			conditions[token.substr(0, eq)] = token.substr(eq + 1);
		}
		return conditions;
	}

} // namespace

namespace onion::voxel
{
	const std::unordered_map<BlockId, std::vector<VariantModel>>& BlockstateRegistry::Get()
	{
		static const std::unordered_map<BlockId, std::vector<VariantModel>> blockstateMap = LoadVariantsModel();
		return blockstateMap;
	}

	std::unordered_map<BlockId, std::vector<VariantModel>> BlockstateRegistry::LoadVariantsModel()
	{
		// Init BlockModel archive first since VariantModel depends on it (same directory, but different file name)
		BlockModel::SetModelArchive(s_ModelArchiveFilePath);

		std::unordered_map<BlockId, std::vector<VariantModel>> blockstateMap;

		ZipArchive archive(s_BlockstateArchiveFilePath);
		std::vector<std::filesystem::path> blockstateFiles = archive.GetFileList();

		for (const auto& blockstateFile : blockstateFiles)
		{
			std::string jsonText = archive.GetFileText(blockstateFile);
			nlohmann::json json = nlohmann::json::parse(jsonText);

			// ---- Ignore multipart blockstates for now ----
			if (!json.contains("variants") || !json.at("variants").is_object())
			{
				continue;
			}

			const std::string blockName = ToBlockName(blockstateFile.filename().string());
			BlockId blockId = BlockIds::GetId(blockName);

			const auto& variants = json.at("variants");

			// ---- Iterate variants ----
			for (auto it = variants.begin(); it != variants.end(); it++)
			{
				const std::string& variantKey = it.key();
				const nlohmann::json& variantValue = it.value();

				auto conditions = ParseConditions(variantKey);

				if (variantValue.is_object())
				{
					VariantModel variantModel = VariantModelFromJson(variantValue);
					variantModel.Conditions = conditions;

					blockstateMap[blockId].push_back(std::move(variantModel));
				}
				else if (variantValue.is_array())
				{
					for (const auto& entry : variantValue)
					{
						VariantModel variantModel = VariantModelFromJson(entry);
						variantModel.Conditions = conditions;

						blockstateMap[blockId].push_back(std::move(variantModel));
					}
				}
			}
		}

		return blockstateMap;
	}

	VariantModel BlockstateRegistry::VariantModelFromJson(const nlohmann::json& json)
	{
		VariantModel variantModel;

		if (json.contains("model") && json.at("model").is_string())
		{
			std::string modelPath = json.at("model").get<std::string>();

			// Strip everithing before "/"
			const size_t slashPos = modelPath.find('/');
			if (slashPos != std::string::npos)
			{
				modelPath = modelPath.substr(slashPos + 1);
			}

			variantModel.Model = BlockModel::FromFile(modelPath);
		}

		if (json.contains("x") && json.at("x").is_number_integer())
		{
			variantModel.RotationX = json.at("x").get<int>();
		}

		if (json.contains("y") && json.at("y").is_number_integer())
		{
			variantModel.RotationY = json.at("y").get<int>();
		}

		if (json.contains("uvlock") && json.at("uvlock").is_boolean())
		{
			variantModel.UVLock = json.at("uvlock").get<bool>();
		}

		return variantModel;
	}
}; // namespace onion::voxel
