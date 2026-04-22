#include "BlockStateJson.hpp"

#include <nlohmann/json.hpp>
#include <renderer/EngineContext.hpp>

#include <sstream>

namespace
{
	static std::string StripNamespace(const std::string& value)
	{
		const size_t pos = value.find(':');
		if (pos != std::string::npos)
			return value.substr(pos + 1);
		return value;
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

	// Parse a single model JSON object into a VariantModel
	static onion::voxel::VariantModel ParseVariantModel(const nlohmann::json& obj)
	{
		onion::voxel::VariantModel vm;
		if (obj.contains("model") && obj["model"].is_string())
			vm.ModelPath = StripNamespace(obj["model"].get<std::string>());
		if (obj.contains("x") && obj["x"].is_number_integer())
			vm.RotationX = obj["x"].get<int>();
		if (obj.contains("y") && obj["y"].is_number_integer())
			vm.RotationY = obj["y"].get<int>();
		if (obj.contains("uvlock") && obj["uvlock"].is_boolean())
			vm.UvLock = obj["uvlock"].get<bool>();
		if (obj.contains("weight") && obj["weight"].is_number_integer())
			vm.Weight = obj["weight"].get<int>();
		return vm;
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

		// ---- "variants" format ----
		if (json.contains("variants") && json["variants"].is_object())
		{
			const auto& variants = json["variants"];

			for (auto it = variants.begin(); it != variants.end(); ++it)
			{
				BlockStateVariant variant;
				variant.Conditions = ParseConditions(it.key());

				const auto& value = it.value();

				if (value.is_object())
				{
					// Single model
					VariantModel vm = ParseVariantModel(value);
					if (!vm.ModelPath.empty())
						variant.Models.push_back(std::move(vm));
				}
				else if (value.is_array())
				{
					// Weighted random models
					for (const auto& elem : value)
					{
						if (!elem.is_object())
							continue;
						VariantModel vm = ParseVariantModel(elem);
						if (!vm.ModelPath.empty())
							variant.Models.push_back(std::move(vm));
					}
				}

				if (!variant.Models.empty())
					blockState.Variants.push_back(std::move(variant));
			}

			if (blockState.Variants.empty())
				throw std::runtime_error("Invalid blockstate: no valid variants found in " + filename);

			return blockState;
		}

		// ---- "multipart" format — not yet supported ----
		if (json.contains("multipart"))
			throw std::runtime_error("Blockstate multipart format not yet supported: " + filename);

		throw std::runtime_error("Invalid blockstate: unrecognised format in " + filename);
	}

} // namespace onion::voxel
