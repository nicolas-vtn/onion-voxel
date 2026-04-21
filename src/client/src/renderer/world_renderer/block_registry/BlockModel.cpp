#include "BlockModel.hpp"

#include <fstream>
#include <stdexcept>

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

		std::array<float, 4> ParseVec4(const nlohmann::json& j)
		{
			return {j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>()};
		}

		void ParseTextures(const nlohmann::json& texturesJson, BlockModel::Textures& textures)
		{
			// Flexible parsing: only read known fields, ignore others
			auto get = [&](const char* key, std::string& target)
			{
				if (texturesJson.contains(key))
				{
					std::string value = texturesJson.at(key).get<std::string>();
					target = StripNamespace(value);
				}
			};

			get("all", textures.All);
			get("particle", textures.Particle);
			get("down", textures.Down);
			get("up", textures.Up);
			get("north", textures.North);
			get("south", textures.South);
			get("west", textures.West);
			get("east", textures.East);
			get("bottom", textures.Bottom);
			get("top", textures.Top);
			get("side", textures.Side);
			get("end", textures.End);
			get("overlay", textures.Overlay);
			get("front", textures.Front);
			get("back", textures.Back);
			get("cross", textures.Cross);
			get("texture", textures.Texture);
		}

		static BlockModel::Face ParseFace(const nlohmann::json& faceJson)
		{
			BlockModel::Face face;

			if (faceJson.contains("uv"))
				face.UV = ParseVec4(faceJson.at("uv"));

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
		return LoadModelRecursive(std::filesystem::path("block") / modelPath);
	}

	std::filesystem::path BlockModel::ResolveModelPath(const std::string& parentPath)
	{
		const std::string clean = StripNamespace(parentPath);
		return std::filesystem::path(clean + ".json");
	}

	void BlockModel::MergeModels(BlockModel& parent, const BlockModel& child)
	{
		// ---- Textures (merge) ----
		auto mergeTex = [](std::string& dst, const std::string& src)
		{
			if (!src.empty())
				dst = src;
		};

		mergeTex(parent.ModelTextures.All, child.ModelTextures.All);
		mergeTex(parent.ModelTextures.Particle, child.ModelTextures.Particle);
		mergeTex(parent.ModelTextures.Down, child.ModelTextures.Down);
		mergeTex(parent.ModelTextures.Up, child.ModelTextures.Up);
		mergeTex(parent.ModelTextures.North, child.ModelTextures.North);
		mergeTex(parent.ModelTextures.South, child.ModelTextures.South);
		mergeTex(parent.ModelTextures.West, child.ModelTextures.West);
		mergeTex(parent.ModelTextures.East, child.ModelTextures.East);
		mergeTex(parent.ModelTextures.Bottom, child.ModelTextures.Bottom);
		mergeTex(parent.ModelTextures.Top, child.ModelTextures.Top);
		mergeTex(parent.ModelTextures.Side, child.ModelTextures.Side);
		mergeTex(parent.ModelTextures.End, child.ModelTextures.End);
		mergeTex(parent.ModelTextures.Overlay, child.ModelTextures.Overlay);
		mergeTex(parent.ModelTextures.Front, child.ModelTextures.Front);
		mergeTex(parent.ModelTextures.Back, child.ModelTextures.Back);
		mergeTex(parent.ModelTextures.Cross, child.ModelTextures.Cross);
		mergeTex(parent.ModelTextures.Texture, child.ModelTextures.Texture);

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

			BlockModel parent = LoadModelRecursive(parentPath);

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

		BlockModel model = BlockModel::FromFile(path);

		auto [insertedIt, _] = s_ModelCache.emplace(path, std::move(model));

		return insertedIt->second;
	}
} // namespace onion::voxel
