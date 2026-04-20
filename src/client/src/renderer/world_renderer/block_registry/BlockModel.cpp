#include "BlockModel.hpp"

#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

namespace onion::voxel
{
	namespace
	{
		static std::string StripNamespace(const std::string& value)
		{
			const size_t pos = value.find(':');
			if (pos != std::string::npos)
				return value.substr(pos + 1);
			return value;
		}

		BlockModel::eParent ParseParent(const std::string& parentStr)
		{
			if (parentStr == "block/cube_all")
				return BlockModel::eParent::CubeAll;

			throw std::runtime_error("Unknown parent: " + parentStr);
		}

		void ParseTextures(const nlohmann::json& texturesJson, BlockModel::Textures& textures)
		{
			// Flexible parsing: only read known fields, ignore others
			if (texturesJson.contains("all"))
			{
				const std::string tex = texturesJson.at("all").get<std::string>();
				textures.All = StripNamespace(tex);
			}

			// Future extension:
			// if (texturesJson.contains("side")) textures.Side = ...
			// if (texturesJson.contains("top"))  textures.Top  = ...
		}
	} // namespace

	BlockModel BlockModel::FromFile(const std::filesystem::path& modelPath)
	{
		// ---- Open file ----
		std::ifstream file(modelPath);
		if (!file.is_open())
			throw std::runtime_error("Failed to open model file: " + modelPath.string());

		// ---- Parse JSON ----
		nlohmann::json json;
		file >> json;

		BlockModel model;

		// ---- Parent ----
		if (json.contains("parent"))
		{
			const std::string parentStrRaw = json.at("parent").get<std::string>();
			const std::string parentStr = StripNamespace(parentStrRaw);
			model.Parent = ParseParent(parentStr);
		}

		// ---- Textures ----
		if (json.contains("textures") && json.at("textures").is_object())
		{
			ParseTextures(json.at("textures"), model.ModelTextures);
		}

		return model;
	}
} // namespace onion::voxel
