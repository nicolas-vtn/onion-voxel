#include "BlockStateJson.hpp"

#include <nlohmann/json.hpp>
#include <renderer/EngineContext.hpp>

namespace
{
	static std::string StripNamespace(const std::string& value)
	{
		const size_t pos = value.find(':');
		if (pos != std::string::npos)
			return value.substr(pos + 1);
		return value;
	}
} // namespace

namespace onion::voxel
{
	BlockStateJson BlockStateJson::FromFile(const std::string& filename)
	{
		BlockStateJson blockState;

		// ---- Read file from ResourcePack ----
		static const std::filesystem::path blockstatesDirectory =
			std::filesystem::path("assets") / "minecraft" / "blockstates";

		const std::string jsonText =
			EngineContext::Get().Assets->GetResourcePackFileText(blockstatesDirectory / filename);

		const auto json = nlohmann::json::parse(jsonText);

		// ---- Validate "variants" ----
		if (!json.contains("variants") || !json["variants"].is_object())
			throw std::runtime_error("Invalid blockstate: missing 'variants' in " + filename);

		const auto& variants = json["variants"];

		// ---- Allowed keys ----
		static const std::unordered_set<std::string> allowedKeys = {
			"", "snowy=false", "snowy=true", "axis=x", "axis=y", "axis=z"};

		// ---- Iterate variants ----
		for (auto it = variants.begin(); it != variants.end(); ++it)
		{
			const std::string& key = it.key();

			// Skip non-allowed keys
			if (allowedKeys.find(key) == allowedKeys.end())
				continue;

			const auto& value = it.value();

			// ---- Case 1: object ----
			if (value.is_object())
			{
				if (value.contains("model") && value["model"].is_string())
				{
					blockState.ModelPath = StripNamespace(value["model"].get<std::string>());
					return blockState;
				}
			}

			// ---- Case 2: array ----
			else if (value.is_array())
			{
				for (const auto& element : value)
				{
					if (!element.is_object())
						continue;

					if (element.contains("model") && element["model"].is_string())
					{
						blockState.ModelPath = StripNamespace(element["model"].get<std::string>());
						return blockState;
					}
				}
			}
		}

		throw std::runtime_error("Invalid blockstate: no valid model found in allowed variants for " + filename);
	}

} // namespace onion::voxel
