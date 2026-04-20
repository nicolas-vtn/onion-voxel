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

		std::array<float, 3> ParseVec3(const nlohmann::json& j)
		{
			return {j[0].get<float>(), j[1].get<float>(), j[2].get<float>()};
		}

		std::array<float, 4> ParseVec4(const nlohmann::json& j)
		{
			return {j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>()};
		}

		BlockModel::eParent ParseParent(const std::string& parentStr)
		{
			if (parentStr == "block/cube_all")
				return BlockModel::eParent::CubeAll;

			if (parentStr == "block/block")
				return BlockModel::eParent::Block;

			throw std::runtime_error("Unknown parent: " + parentStr);
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
			get("bottom", textures.Bottom);
			get("top", textures.Top);
			get("side", textures.Side);
			get("overlay", textures.Overlay);
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
					elem.From = ParseVec3(elemJson.at("from"));

				if (elemJson.contains("to"))
					elem.To = ParseVec3(elemJson.at("to"));

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

		// ---- Elements ----
		if (json.contains("elements") && json["elements"].is_array())
		{
			ParseElements(json["elements"], model.Elements);
		}

		return model;
	}
} // namespace onion::voxel
