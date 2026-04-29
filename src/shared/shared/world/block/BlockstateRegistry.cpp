#include "BlockstateRegistry.hpp"

#include <set>
#include <sstream>

#include <shared/zip_archive/ZipArchive.hpp>

namespace
{
	struct ApplySpec
	{
		std::string ModelPath;
		int RotationX = 0;
		int RotationY = 0;
		bool UVLock = false;
	};

	struct WhenPredicate
	{
		enum class Type
		{
			MatchAll,
			PropertyEqualsAny,
			And,
			Or,
		};

		Type PredicateType = Type::MatchAll;
		std::string PropertyKey;
		std::set<std::string> PropertyValues;
		std::vector<WhenPredicate> Children;
	};

	struct MultipartEntry
	{
		WhenPredicate When;
		std::vector<ApplySpec> Applies;
	};

	static std::string JsonScalarToString(const nlohmann::json& value)
	{
		if (value.is_string())
			return value.get<std::string>();
		if (value.is_boolean())
			return value.get<bool>() ? "true" : "false";
		if (value.is_number_integer())
			return std::to_string(value.get<int>());
		if (value.is_number_unsigned())
			return std::to_string(value.get<unsigned int>());
		if (value.is_number_float())
			return std::to_string(value.get<float>());
		return value.dump();
	}

	static std::set<std::string> ParseOrSeparatedValues(const std::string& value)
	{
		std::set<std::string> values;
		std::istringstream stream(value);
		std::string token;
		while (std::getline(stream, token, '|'))
		{
			if (!token.empty())
				values.insert(token);
		}
		if (values.empty())
			values.insert(value);
		return values;
	}

	static std::string StripModelNamespace(const std::string& modelPath)
	{
		std::string stripped = modelPath;

		const size_t namespacePos = stripped.find(':');
		if (namespacePos != std::string::npos)
			stripped = stripped.substr(namespacePos + 1);

		const std::string blockPrefix = "block/";
		if (stripped.rfind(blockPrefix, 0) == 0)
			stripped = stripped.substr(blockPrefix.size());

		return stripped;
	}

	static ApplySpec ApplySpecFromJson(const nlohmann::json& applyJson)
	{
		ApplySpec spec;

		if (applyJson.contains("model") && applyJson.at("model").is_string())
			spec.ModelPath = StripModelNamespace(applyJson.at("model").get<std::string>());

		if (applyJson.contains("x") && applyJson.at("x").is_number_integer())
			spec.RotationX = applyJson.at("x").get<int>();

		if (applyJson.contains("y") && applyJson.at("y").is_number_integer())
			spec.RotationY = applyJson.at("y").get<int>();

		if (applyJson.contains("uvlock") && applyJson.at("uvlock").is_boolean())
			spec.UVLock = applyJson.at("uvlock").get<bool>();

		return spec;
	}

	static std::vector<ApplySpec> ParseApplySpecs(const nlohmann::json& applyJson)
	{
		std::vector<ApplySpec> applies;

		if (applyJson.is_object())
		{
			applies.push_back(ApplySpecFromJson(applyJson));
		}
		else if (applyJson.is_array() && !applyJson.empty() && applyJson.front().is_object())
		{
			applies.push_back(ApplySpecFromJson(applyJson.front()));
		}

		return applies;
	}

	static WhenPredicate ParseWhenPredicate(const nlohmann::json& whenJson)
	{
		WhenPredicate predicate;

		if (whenJson.is_null())
		{
			predicate.PredicateType = WhenPredicate::Type::MatchAll;
			return predicate;
		}

		if (!whenJson.is_object())
		{
			predicate.PredicateType = WhenPredicate::Type::MatchAll;
			return predicate;
		}

		if (whenJson.contains("OR") && whenJson.at("OR").is_array())
		{
			predicate.PredicateType = WhenPredicate::Type::Or;
			for (const auto& entry : whenJson.at("OR"))
				predicate.Children.push_back(ParseWhenPredicate(entry));
			return predicate;
		}

		if (whenJson.contains("AND") && whenJson.at("AND").is_array())
		{
			predicate.PredicateType = WhenPredicate::Type::And;
			for (const auto& entry : whenJson.at("AND"))
				predicate.Children.push_back(ParseWhenPredicate(entry));
			return predicate;
		}

		predicate.PredicateType = WhenPredicate::Type::And;
		for (const auto& [key, value] : whenJson.items())
		{
			WhenPredicate propPredicate;
			propPredicate.PredicateType = WhenPredicate::Type::PropertyEqualsAny;
			propPredicate.PropertyKey = key;
			propPredicate.PropertyValues = ParseOrSeparatedValues(JsonScalarToString(value));
			predicate.Children.push_back(std::move(propPredicate));
		}

		if (predicate.Children.empty())
			predicate.PredicateType = WhenPredicate::Type::MatchAll;

		return predicate;
	}

	static bool EvaluateWhenPredicate(const WhenPredicate& predicate,
									  const std::map<std::string, std::string>& properties)
	{
		switch (predicate.PredicateType)
		{
			case WhenPredicate::Type::MatchAll:
				return true;
			case WhenPredicate::Type::PropertyEqualsAny:
				{
					auto it = properties.find(predicate.PropertyKey);
					if (it == properties.end())
						return false;
					return predicate.PropertyValues.contains(it->second);
				}
			case WhenPredicate::Type::And:
				{
					for (const auto& child : predicate.Children)
						if (!EvaluateWhenPredicate(child, properties))
							return false;
					return true;
				}
			case WhenPredicate::Type::Or:
				{
					for (const auto& child : predicate.Children)
						if (EvaluateWhenPredicate(child, properties))
							return true;
					return false;
				}
		}

		return true;
	}

	static void CollectPropertyDomainFromPredicate(const WhenPredicate& predicate,
												   std::map<std::string, std::set<std::string>>& propertyDomain)
	{
		if (predicate.PredicateType == WhenPredicate::Type::PropertyEqualsAny)
		{
			auto& values = propertyDomain[predicate.PropertyKey];
			values.insert(predicate.PropertyValues.begin(), predicate.PropertyValues.end());
			return;
		}

		for (const auto& child : predicate.Children)
			CollectPropertyDomainFromPredicate(child, propertyDomain);
	}

	static std::string BuildPropertiesSignature(const std::map<std::string, std::string>& properties)
	{
		std::string signature;
		for (const auto& [key, value] : properties)
		{
			if (!signature.empty())
				signature.push_back(',');
			signature += key;
			signature.push_back('=');
			signature += value;
		}
		return signature;
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

	// Rotate a UV rect [u1,v1,u2,v2] by `steps` × 90° CW around center (8,8).
	// Used for top/bottom faces under Y rotation, and east/west faces under X rotation.
	static std::array<float, 4> RotateUV90CW(const std::array<float, 4>& uv)
	{
		// Corners: TL=(u1,v1), TR=(u2,v1), BR=(u2,v2), BL=(u1,v2)
		// 90° CW around (8,8): (u,v) → (16-v, u)  [in MC UV space]
		// New rect from rotated corners:
		float u1 = uv[0], v1 = uv[1], u2 = uv[2], v2 = uv[3];
		// After 90° CW: new u-range = [16-v2, 16-v1], new v-range = [u1, u2]
		return {16.f - v2, u1, 16.f - v1, u2};
	}

	static std::array<float, 4> RotateUVSteps(const std::array<float, 4>& uv, int steps)
	{
		steps = ((steps % 4) + 4) % 4;
		std::array<float, 4> result = uv;
		for (int i = 0; i < steps; i++)
			result = RotateUV90CW(result);
		return result;
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

			// Remap face direction keys and rotate UVs for faces whose plane is
			// parallel to the rotation axis (i.e. faces that stay on the same side).
			//
			// - Y rotation: up/down faces stay in place → their UV rotates by stepsY CW.
			// - X rotation: east/west faces stay in place → their UV rotates by stepsX CW.
			// Side faces that get remapped to a new direction keep their UV content
			// unchanged (the texture moves with the geometry).
			std::unordered_map<std::string, onion::voxel::BlockModel::Face> rotatedFaces;
			for (auto& [faceName, face] : elem.Faces)
			{
				std::string newName = faceName;
				if (stepsX != 0)
					newName = RotateFaceX(newName, stepsX);
				if (stepsY != 0)
					newName = RotateFaceY(newName, stepsY);

				// If the face was NOT remapped by Y rotation (up/down), rotate its UV.
				if (stepsY != 0 && newName == faceName && (faceName == "up" || faceName == "down"))
				{
					face.UV = RotateUVSteps(face.UV, stepsY);
				}

				// If the face was NOT remapped by X rotation (east/west), rotate its UV.
				if (stepsX != 0 && newName == faceName && (faceName == "east" || faceName == "west"))
				{
					face.UV = RotateUVSteps(face.UV, stepsX);
				}

				rotatedFaces[newName] = std::move(face);
			}
			elem.Faces = std::move(rotatedFaces);
		}

		return;
	}

	static onion::voxel::VariantModel ComposeMultipartVariant(const std::vector<ApplySpec>& applies,
															  const std::map<std::string, std::string>& properties)
	{
		using namespace onion::voxel;
		VariantModel variant;
		variant.Properties = properties;
		variant.Model.AmbientOcclusion = true;

		for (const auto& apply : applies)
		{
			if (apply.ModelPath.empty())
				continue;

			BlockModel model = BlockModel::FromFile(apply.ModelPath);

			const int stepsX = ((apply.RotationX / 90) % 4 + 4) % 4;
			const int stepsY = ((apply.RotationY / 90) % 4 + 4) % 4;
			if (stepsX != 0 || stepsY != 0)
				RotateModel(model, stepsX, stepsY);

			variant.Model.AmbientOcclusion = variant.Model.AmbientOcclusion && model.AmbientOcclusion;

			for (const auto& [textureKey, textureValue] : model.ModelTextures)
				variant.Model.ModelTextures[textureKey] = textureValue;

			variant.Model.Elements.insert(variant.Model.Elements.end(),
										  std::make_move_iterator(model.Elements.begin()),
										  std::make_move_iterator(model.Elements.end()));
		}

		variant.RotationX = 0;
		variant.RotationY = 0;
		variant.UVLock = false;
		return variant;
	}

	template <typename F>
	static void
	EnumerateAssignmentsRecursive(const std::vector<std::pair<std::string, std::vector<std::string>>>& dimensions,
								  size_t index,
								  std::map<std::string, std::string>& current,
								  F&& callback)
	{
		if (index >= dimensions.size())
		{
			callback(current);
			return;
		}

		const auto& [key, values] = dimensions[index];
		for (const auto& value : values)
		{
			current[key] = value;
			EnumerateAssignmentsRecursive(dimensions, index + 1, current, callback);
		}
	}

	template <typename F>
	static void EnumerateAssignments(const std::map<std::string, std::set<std::string>>& propertyDomain, F&& callback)
	{
		if (propertyDomain.empty())
		{
			std::map<std::string, std::string> empty;
			callback(empty);
			return;
		}

		std::vector<std::pair<std::string, std::vector<std::string>>> dimensions;
		dimensions.reserve(propertyDomain.size());
		for (const auto& [key, values] : propertyDomain)
		{
			dimensions.emplace_back(key, std::vector<std::string>(values.begin(), values.end()));
		}

		std::map<std::string, std::string> current;
		EnumerateAssignmentsRecursive(dimensions, 0, current, callback);
	}

	static std::vector<MultipartEntry> ParseMultipartEntries(const nlohmann::json& multipartJson)
	{
		std::vector<MultipartEntry> entries;
		if (!multipartJson.is_array())
			return entries;

		for (const auto& part : multipartJson)
		{
			if (!part.is_object() || !part.contains("apply"))
				continue;

			MultipartEntry entry;
			entry.Applies = ParseApplySpecs(part.at("apply"));
			if (entry.Applies.empty())
				continue;

			if (part.contains("when"))
				entry.When = ParseWhenPredicate(part.at("when"));
			else
				entry.When.PredicateType = WhenPredicate::Type::MatchAll;

			entries.push_back(std::move(entry));
		}

		return entries;
	}

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
	static std::map<std::string, std::string> ParseProperties(const std::string& key)
	{
		std::map<std::string, std::string> properties;
		if (key.empty())
			return properties;

		std::istringstream stream(key);
		std::string token;
		while (std::getline(stream, token, ','))
		{
			const size_t eq = token.find('=');
			if (eq == std::string::npos)
				continue;
			properties[token.substr(0, eq)] = token.substr(eq + 1);
		}
		return properties;
	}

} // namespace

namespace onion::voxel
{
	const std::unordered_map<BlockId, std::vector<VariantModel>>& BlockstateRegistry::Get()
	{
		static const std::unordered_map<BlockId, std::vector<VariantModel>> blockstateMap = LoadVariantsModel();
		return blockstateMap;
	}

	bool BlockstateRegistry::IsTallPlant(BlockId blockId)
	{
		auto& blockstateMap = Get();

		auto it = blockstateMap.find(blockId);
		if (it != blockstateMap.end())
		{
			bool containsLower = false;
			bool containsUpper = false;
			for (const auto& variant : it->second)
			{
				auto itvariant = variant.Properties.find("half");
				if (itvariant != variant.Properties.end())
				{
					if (itvariant->second == "lower")
						containsLower = true;
					else if (itvariant->second == "upper")
						containsUpper = true;
				}
			}

			return containsLower && containsUpper;
		}

		return false;
	}

	uint8_t BlockstateRegistry::GetVariantIndex(BlockId id, const std::map<std::string, std::string>& properties)
	{
		auto& blockstateMap = Get();

		auto it = blockstateMap.find(id);
		if (it != blockstateMap.end())
		{
			// Searches for the first variant that fits all the given properties.
			const auto& variants = it->second;
			for (size_t i = 0; i < variants.size(); i++)
			{
				const auto& variant = variants[i];
				bool matches = true;

				for (const auto& [key, value] : variant.Properties)
				{
					auto itprop = properties.find(key);
					if (itprop == properties.end() || itprop->second != value)
					{
						matches = false;
						break;
					}
				}

				if (matches)
					return static_cast<uint8_t>(i);
			}
		}

		return 0;
	}

	bool BlockstateRegistry::CountsInAO(BlockId id, uint8_t variantIndex)
	{
		auto& blockstateMap = Get();

		if (id == BlockId::Air)
			return false;

		auto it = blockstateMap.find(id);
		if (it != blockstateMap.end())
		{
			return it->second[variantIndex].Model.AmbientOcclusion;
		}

		return false;
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

			const std::string blockName = ToBlockName(blockstateFile.filename().string());
			BlockId blockId = BlockIds::GetId(blockName);

			if (json.contains("variants") && json.at("variants").is_object())
			{
				const auto& variants = json.at("variants");

				// ---- Iterate variants ----
				for (auto it = variants.begin(); it != variants.end(); it++)
				{
					const std::string& variantKey = it.key();
					const nlohmann::json& variantValue = it.value();

					auto properties = ParseProperties(variantKey);

					if (variantValue.is_object())
					{
						VariantModel variantModel = VariantModelFromJson(variantValue);
						variantModel.Properties = properties;

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
							variantModel.Properties = properties;

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
			else if (json.contains("multipart") && json.at("multipart").is_array())
			{
				const std::vector<MultipartEntry> multipartEntries = ParseMultipartEntries(json.at("multipart"));
				if (multipartEntries.empty())
					continue;

				std::map<std::string, std::set<std::string>> propertyDomain;
				for (const auto& entry : multipartEntries)
					CollectPropertyDomainFromPredicate(entry.When, propertyDomain);

				std::set<std::string> seenSignatures;
				EnumerateAssignments(propertyDomain,
									 [&](const std::map<std::string, std::string>& assignment)
									 {
										 std::vector<ApplySpec> matchedApplies;
										 for (const auto& entry : multipartEntries)
										 {
											 if (!EvaluateWhenPredicate(entry.When, assignment))
												 continue;
											 matchedApplies.insert(
												 matchedApplies.end(), entry.Applies.begin(), entry.Applies.end());
										 }

										 if (matchedApplies.empty())
											 return;

										 const std::string signature = BuildPropertiesSignature(assignment);
										 if (!seenSignatures.insert(signature).second)
											 return;

										 VariantModel variant = ComposeMultipartVariant(matchedApplies, assignment);
										 if (!variant.Model.Elements.empty())
											 blockstateMap[blockId].push_back(std::move(variant));
									 });
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
