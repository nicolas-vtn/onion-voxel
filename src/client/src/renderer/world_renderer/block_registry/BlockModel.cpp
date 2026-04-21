#include "BlockModel.hpp"

#include <nlohmann/json.hpp>

#include <renderer/EngineContext.hpp>

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

		std::array<uint8_t, 3> ParseiVec3(const nlohmann::json& j)
		{
			return {j[0].get<uint8_t>(), j[1].get<uint8_t>(), j[2].get<uint8_t>()};
		}

		std::array<uint8_t, 4> ParseiVec4(const nlohmann::json& j)
		{
			return {j[0].get<uint8_t>(), j[1].get<uint8_t>(), j[2].get<uint8_t>(), j[3].get<uint8_t>()};
		}

		void ParseTextures(const nlohmann::json& texturesJson, BlockModel::Textures& textures)
		{
			for (const auto& [key, val] : texturesJson.items())
			{
				if (val.is_string())
					textures[key] = StripNamespace(val.get<std::string>());
			}
		}

		static BlockModel::Face ParseFace(const nlohmann::json& faceJson)
		{
			BlockModel::Face face;

			if (faceJson.contains("uv"))
				face.UV = ParseiVec4(faceJson.at("uv"));

			if (faceJson.contains("texture"))
				face.Texture = faceJson.at("texture").get<std::string>();

			if (faceJson.contains("cullface"))
				face.CullFace = faceJson.at("cullface").get<std::string>();

			if (faceJson.contains("tintindex"))
				face.TintIndex = faceJson.at("tintindex").get<int>();

			return face;
		}

		static void ParseElements(const nlohmann::json& elementsJson, std::vector<BlockModel::Element>& elements)
		{
			for (const auto& elemJson : elementsJson)
			{
				BlockModel::Element elem;

				if (elemJson.contains("from"))
					elem.From = ParseiVec3(elemJson.at("from"));

				if (elemJson.contains("to"))
					elem.To = ParseiVec3(elemJson.at("to"));

				if (elemJson.contains("faces"))
				{
					for (auto& [faceName, faceJson] : elemJson.at("faces").items())
					{
						elem.Faces[faceName] = ParseFace(faceJson);
					}
				}

				elements.push_back(std::move(elem));
			}
		}
	} // namespace

	BlockModel BlockModel::FromFile(const std::filesystem::path& modelPath)
	{
		const std::filesystem::path fullPath = std::filesystem::path("block") / modelPath;
		return GetModel(fullPath);
	}

	std::filesystem::path BlockModel::ResolveModelPath(const std::string& parentPath)
	{
		const std::string clean = StripNamespace(parentPath);
		return std::filesystem::path(clean + ".json");
	}

	void BlockModel::MergeModels(BlockModel& parent, const BlockModel& child)
	{
		// ---- Textures (child entries override parent, non-empty values only) ----
		for (const auto& [key, val] : child.ModelTextures)
		{
			if (!val.empty())
				parent.ModelTextures[key] = val;
		}

		// ---- Elements ----
		if (!child.Elements.empty())
		{
			parent.Elements = child.Elements; // override completely
		}
	}

	BlockModel BlockModel::LoadRawModel(const std::filesystem::path& modelPath)
	{
		// ---- Read file from ResourcePack ----
		static const std::filesystem::path modelsDirectory = std::filesystem::path("assets") / "minecraft" / "models";
		const std::string jsonText = EngineContext::Get().Assets->GetResourcePackFileText(modelsDirectory / modelPath);

		// ---- Parse JSON ----
		nlohmann::json json = nlohmann::json::parse(jsonText);

		BlockModel model;

		// ---- Parent ----
		if (json.contains("parent") && json.at("parent").is_string())
		{
			model.ParentPath = json.at("parent").get<std::string>();
		}

		// ---- Textures ----
		if (json.contains("textures") && json.at("textures").is_object())
		{
			ParseTextures(json.at("textures"), model.ModelTextures);
		}

		// ---- Elements ----
		if (json.contains("elements") && json["elements"].is_array())
		{
			ParseElements(json["elements"], model.Elements);
		}

		return model;
	}

	BlockModel BlockModel::LoadModelRecursive(const std::filesystem::path& modelPath)
	{
		BlockModel model = LoadRawModel(modelPath);

		if (!model.ParentPath.empty())
		{
			std::filesystem::path parentPath = ResolveModelPath(model.ParentPath);

			BlockModel parent = GetModel(parentPath);

			MergeModels(parent, model);
			return parent; // merged result stored in parent
		}

		return model;
	}

	const BlockModel& BlockModel::GetModel(const std::filesystem::path& path)
	{
		std::unique_lock lock(s_CacheMutex);

		auto it = s_ModelCache.find(path);
		if (it != s_ModelCache.end())
			return it->second;

		BlockModel model = BlockModel::LoadModelRecursive(path);

		auto [insertedIt, inserted] = s_ModelCache.emplace(path, std::move(model));

		return insertedIt->second;
	}

	void BlockModel::ClearCache()
	{
		std::unique_lock lock(s_CacheMutex);
		s_ModelCache.clear();
	}
} // namespace onion::voxel
