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

	// ---------------------------------------------------------------------------
	// Geometry rotation helpers (for baking blockstate x/y rotations)
	// ---------------------------------------------------------------------------

	// Rotate a point 90° CW around center 8 on the X axis (looking from +X).
	// Cycle: Up → South → Down → North → Up
	static glm::vec3 RotateX90(glm::vec3 p)
	{
		float y = p.y - 8;
		float z = p.z - 8;
		return {p.x, z + 8, -y + 8};
	}

	// Rotate a point 90° CW around center 8 on the Y axis (looking down).
	// Cycle: North(-Z) → East(+X) → South(+Z) → West(-X) → North
	static glm::vec3 RotateY90(glm::vec3 p)
	{
		float x = p.x - 8;
		float z = p.z - 8;
		return {-z + 8, p.y, x + 8};
	}

	static glm::vec3 RotatePoint(glm::vec3 p, int steps, bool aroundY)
	{
		steps = ((steps % 4) + 4) % 4;
		for (int i = 0; i < steps; i++)
			p = aroundY ? RotateY90(p) : RotateX90(p);
		return p;
	}

	// Remap face names for Y-axis rotation (CW from above):
	// north(0) → east(1) → south(2) → west(3) → north
	static std::string RotateFaceY(const std::string& face, int steps)
	{
		static const char* cycle[4] = {"north", "east", "south", "west"};
		int idx = -1;
		if (face == "north")
			idx = 0;
		else if (face == "east")
			idx = 1;
		else if (face == "south")
			idx = 2;
		else if (face == "west")
			idx = 3;
		if (idx < 0)
			return face; // up / down unaffected
		return cycle[((idx + steps) % 4 + 4) % 4];
	}

	// Remap face names for X-axis rotation (CW from +X):
	// up(0) → south(1) → down(2) → north(3) → up
	static std::string RotateFaceX(const std::string& face, int steps)
	{
		static const char* cycle[4] = {"up", "south", "down", "north"};
		int idx = -1;
		if (face == "up")
			idx = 0;
		else if (face == "south")
			idx = 1;
		else if (face == "down")
			idx = 2;
		else if (face == "north")
			idx = 3;
		if (idx < 0)
			return face; // east / west unaffected
		return cycle[((idx + steps) % 4 + 4) % 4];
	}

	static void RotateModel(onion::voxel::BlockModel& model, int stepsX, int stepsY)
	{
		if (stepsX == 0 && stepsY == 0)
			return;

		for (auto& elem : model.Elements)
		{
			glm::vec3 from = elem.From;
			glm::vec3 to = elem.To;

			// Apply X rotation first, then Y
			from = RotatePoint(from, stepsX, false);
			to = RotatePoint(to, stepsX, false);
			from = RotatePoint(from, stepsY, true);
			to = RotatePoint(to, stepsY, true);

			// Re-normalise min/max after rotation
			elem.From = {std::min(from.x, to.x), std::min(from.y, to.y), std::min(from.z, to.z)};
			elem.To = {std::max(from.x, to.x), std::max(from.y, to.y), std::max(from.z, to.z)};

			// Remap face direction keys
			std::unordered_map<std::string, onion::voxel::BlockModel::Face> rotatedFaces;
			for (auto& [faceName, face] : elem.Faces)
			{
				std::string newName = faceName;
				if (stepsX != 0)
					newName = RotateFaceX(newName, stepsX);
				if (stepsY != 0)
					newName = RotateFaceY(newName, stepsY);
				rotatedFaces[newName] = std::move(face);
			}
			elem.Faces = std::move(rotatedFaces);
		}

		return;
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

					// ----- Bake blockstate rotation into element geometry -----
					const int stepsX = ((variantModel.RotationX / 90) % 4 + 4) % 4;
					const int stepsY = ((variantModel.RotationY / 90) % 4 + 4) % 4;
					if (stepsX != 0 || stepsY != 0)
						RotateModel(variantModel.Model, stepsX, stepsY);

					// Resets rotation because it's already baked into the model geometry.
					variantModel.RotationX = 0;
					variantModel.RotationY = 0;

					blockstateMap[blockId].push_back(std::move(variantModel));
				}
				else if (variantValue.is_array())
				{
					for (const auto& entry : variantValue)
					{
						VariantModel variantModel = VariantModelFromJson(entry);
						variantModel.Conditions = conditions;

						// ----- Bake blockstate rotation into element geometry -----
						const int stepsX = ((variantModel.RotationX / 90) % 4 + 4) % 4;
						const int stepsY = ((variantModel.RotationY / 90) % 4 + 4) % 4;
						if (stepsX != 0 || stepsY != 0)
							RotateModel(variantModel.Model, stepsX, stepsY);

						// Resets rotation because it's already baked into the model geometry.
						variantModel.RotationX = 0;
						variantModel.RotationY = 0;

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
