#include "BlockModel.hpp"

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

		void ParseVec3(const nlohmann::json& j, glm::vec3& vec)
		{
			vec = {j[0].get<float>(), j[1].get<float>(), j[2].get<float>()};
		}

		void ParseVec4(const nlohmann::json& j, std::array<float, 4>& vec)
		{
			vec = {j[0].get<float>(), j[1].get<float>(), j[2].get<float>(), j[3].get<float>()};
		}

		void ParseTextures(const nlohmann::json& texturesJson, BlockModel::Textures& textures)
		{
			for (const auto& [key, val] : texturesJson.items())
			{
				if (val.is_string())
					textures[key] = StripNamespace(val.get<std::string>());
			}
		}

		static void ParseFace(const nlohmann::json& faceJson, BlockModel::Face& face)
		{
			if (faceJson.contains("uv"))
				ParseVec4(faceJson.at("uv"), face.UV);

			if (faceJson.contains("texture"))
				face.Texture = faceJson.at("texture").get<std::string>();

			if (faceJson.contains("cullface"))
				face.CullFace = faceJson.at("cullface").get<std::string>();

			if (faceJson.contains("tintindex"))
				face.TintIndex = faceJson.at("tintindex").get<int>();

			if (faceJson.contains("rotation"))
				face.Rotation = faceJson.at("rotation").get<float>();
		}

		static void ParseElementRotation(const nlohmann::json& rotationJson, BlockModel::ElementRotation& rotation)
		{
			if (rotationJson.contains("origin"))
				ParseVec3(rotationJson.at("origin"), rotation.Origin);

			if (rotationJson.contains("axis"))
				rotation.Axis = rotationJson.at("axis").get<std::string>();

			if (rotationJson.contains("angle"))
				rotation.Angle = rotationJson.at("angle").get<float>();

			if (rotationJson.contains("rescale"))
				rotation.Rescale = rotationJson.at("rescale").get<bool>();
		}

		static void ParseElements(const nlohmann::json& elementsJson, std::vector<BlockModel::Element>& elements)
		{
			for (const auto& elemJson : elementsJson)
			{
				BlockModel::Element elem;

				if (elemJson.contains("from"))
					ParseVec3(elemJson.at("from"), elem.From);

				if (elemJson.contains("to"))
					ParseVec3(elemJson.at("to"), elem.To);

				if (elemJson.contains("rotation"))
					ParseElementRotation(elemJson.at("rotation"), elem.Rotation);

				if (elemJson.contains("shade"))
					elem.Shade = elemJson.at("shade").get<bool>();

				if (elemJson.contains("faces"))
				{
					for (auto& [faceName, faceJson] : elemJson.at("faces").items())
					{
						BlockModel::Face face;

						// Default UVs derived from element bounds per face direction,
						// following Minecraft's convention (V=0 at top of texture).
						const float fx = elem.From.x, fy = elem.From.y, fz = elem.From.z;
						const float tx = elem.To.x, ty = elem.To.y, tz = elem.To.z;
						if (faceName == "down")
							face.UV = {fx, fz, tx, tz};
						else if (faceName == "up")
							face.UV = {fx, fz, tx, tz};
						else if (faceName == "north")
							face.UV = {16 - tx, 16 - ty, 16 - fx, 16 - fy};
						else if (faceName == "south")
							face.UV = {fx, 16 - ty, tx, 16 - fy};
						else if (faceName == "west")
							face.UV = {fz, 16 - ty, tz, 16 - fy};
						else if (faceName == "east")
							face.UV = {16 - tz, 16 - ty, 16 - fz, 16 - fy};

						ParseFace(faceJson, face);

						elem.Faces[faceName] = face;
					}
				}

				elements.push_back(std::move(elem));
			}
		}

		void ParseDisplayInfo(const nlohmann::json& displayJson, BlockModel::DisplayInfo& displayInfo)
		{
			if (displayJson.contains("rotation"))
				ParseVec3(displayJson.at("rotation"), displayInfo.Rotation);
			if (displayJson.contains("translation"))
				ParseVec3(displayJson.at("translation"), displayInfo.Translation);
			if (displayJson.contains("scale"))
				ParseVec3(displayJson.at("scale"), displayInfo.Scale);
		}

		void ParseDisplay(const nlohmann::json& displayJson, BlockModel::Display& display)
		{
			if (displayJson.contains("gui"))
				ParseDisplayInfo(displayJson.at("gui"), display.Gui);
		}

	} // namespace

	void BlockModel::SetModelArchive(const std::filesystem::path& archiveFilePath)
	{
		s_ModelArchive = std::make_unique<ZipArchive>(archiveFilePath);
	}

	BlockModel BlockModel::FromFile(const std::string& filename)
	{
		std::filesystem::path pathInsideArchive;
		if (filename.rfind("block/", 0) == 0 || filename.rfind("item/", 0) == 0)
			pathInsideArchive = std::filesystem::path(filename);
		else
			pathInsideArchive = std::filesystem::path("block") / filename;
		return GetModel(pathInsideArchive);
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
		if (modelPath.string().find("builtin/") == 0)
		{
			BlockModel model;
			model.AmbientOcclusion = false;

			BlockModel::Element elem;
			elem.From = {0.0f, 0.0f, 8.0f};
			elem.To = {16.0f, 16.0f, 8.0f}; // zero-thickness plane

			BlockModel::Face face;
			face.Texture = "#layer0";
			face.UV = {0, 0, 16, 16};

			// Front/back only to keep it flat
			elem.Faces["north"] = face;
			elem.Faces["south"] = face;

			model.Elements.push_back(std::move(elem));

			// Keep GUI transform neutral (no forced rotation/scale)
			model.ModelDisplay.Gui.Rotation = {0.0f, 0.0f, 0.0f};
			model.ModelDisplay.Gui.Translation = {0.0f, 0.0f, 0.0f};
			model.ModelDisplay.Gui.Scale = {1.0f, 1.0f, 1.0f};

			return model;
		}

		// ---- Read file from Archive ----
		std::filesystem::path path = modelPath;
		path.replace_extension(".json");
		const std::string jsonText = s_ModelArchive->GetFileText(path);

		// ---- Parse JSON ----
		nlohmann::json json = nlohmann::json::parse(jsonText);

		BlockModel model;

		// ---- Parent ----
		if (json.contains("parent") && json.at("parent").is_string())
		{
			model.ParentPath = json.at("parent").get<std::string>();
		}

		// ---- Ambient Occlusion ----
		if (json.contains("ambientocclusion") && json.at("ambientocclusion").is_boolean())
		{
			model.AmbientOcclusion = json.at("ambientocclusion").get<bool>();
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

		// ---- Display ----
		if (json.contains("display") && json["display"].is_object())
		{
			ParseDisplay(json["display"], model.ModelDisplay);
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
