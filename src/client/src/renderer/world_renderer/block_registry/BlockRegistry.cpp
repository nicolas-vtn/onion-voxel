#include "BlockRegistry.hpp"

#include "BlockModel.hpp"
#include "BlockStateJson.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>

// ---------------------------------------------------------------------------
// Anonymous namespace — file-local helpers
// ---------------------------------------------------------------------------
namespace
{
	static std::string ToFilename(const std::string& texturePath)
	{
		return texturePath + ".png";
	}

	static std::string ToBlockName(const std::string& blockstateName)
	{
		std::string name = blockstateName;

		// ---- Remove ".json" extension ----
		const std::string ext = ".json";
		if (name.size() >= ext.size() && name.substr(name.size() - ext.size()) == ext)
			name.erase(name.size() - ext.size());

		// ---- Replace '_' with ' ' ----
		std::replace(name.begin(), name.end(), '_', ' ');

		// ---- Capitalize each word ----
		bool capitalizeNext = true;
		for (char& c : name)
		{
			if (std::isspace(static_cast<unsigned char>(c)))
				capitalizeNext = true;
			else if (capitalizeNext)
			{
				c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
				capitalizeNext = false;
			}
			else
				c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		}

		return name;
	}

	static std::string ResolveTexture(const std::string& ref, const onion::voxel::BlockModel::Textures& t)
	{
		std::function<std::string(const std::string&)> resolve = [&](const std::string& r) -> std::string
		{
			if (!r.starts_with('#'))
				return ToFilename(r);
			const std::string key = r.substr(1);
			auto it = t.find(key);
			if (it != t.end())
				return resolve(it->second);
			return "";
		};
		return resolve(ref);
	}

	static onion::voxel::Face ToFace(const std::string& name)
	{
		using namespace onion::voxel;
		if (name == "up")
			return Face::Top;
		if (name == "down")
			return Face::Bottom;
		if (name == "north")
			return Face::Front;
		if (name == "south")
			return Face::Back;
		if (name == "west")
			return Face::Left;
		if (name == "east")
			return Face::Right;
		throw std::runtime_error("Unknown face: " + name);
	}

	static bool IsOverlayElem(const onion::voxel::BlockModel::Element& elem)
	{
		for (const auto& [faceName, face] : elem.Faces)
			if (face.Texture != "#overlay")
				return false;
		return true;
	}

	// ---------------------------------------------------------------------------
	// Geometry rotation helpers (for baking blockstate x/y rotations)
	// ---------------------------------------------------------------------------

	// Rotate a point 90° CW around center 8 on the X axis (looking from +X).
	// Cycle: Up → South → Down → North → Up
	static glm::u8vec3 RotateX90(glm::u8vec3 p)
	{
		int y = p.y - 8;
		int z = p.z - 8;
		return {p.x, static_cast<uint8_t>(z + 8), static_cast<uint8_t>(-y + 8)};
	}

	// Rotate a point 90° CW around center 8 on the Y axis (looking down).
	// Cycle: North(-Z) → East(+X) → South(+Z) → West(-X) → North
	static glm::u8vec3 RotateY90(glm::u8vec3 p)
	{
		int x = p.x - 8;
		int z = p.z - 8;
		return {static_cast<uint8_t>(-z + 8), p.y, static_cast<uint8_t>(x + 8)};
	}

	static glm::u8vec3 RotatePoint(glm::u8vec3 p, int steps, bool aroundY)
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

	// Apply X then Y rotation to a BlockModel's element geometry and face name keys.
	// stepsX / stepsY = rotation_degrees / 90
	static onion::voxel::BlockModel ApplyModelRotation(onion::voxel::BlockModel model, int stepsX, int stepsY)
	{
		if (stepsX == 0 && stepsY == 0)
			return model;

		for (auto& elem : model.Elements)
		{
			glm::u8vec3 from(elem.From[0], elem.From[1], elem.From[2]);
			glm::u8vec3 to(elem.To[0], elem.To[1], elem.To[2]);

			// Apply X rotation first, then Y
			from = RotatePoint(from, stepsX, false);
			to = RotatePoint(to, stepsX, false);
			from = RotatePoint(from, stepsY, true);
			to = RotatePoint(to, stepsY, true);

			// Re-normalise min/max after rotation
			elem.From = {static_cast<uint8_t>(std::min(from.x, to.x)),
						 static_cast<uint8_t>(std::min(from.y, to.y)),
						 static_cast<uint8_t>(std::min(from.z, to.z))};
			elem.To = {static_cast<uint8_t>(std::max(from.x, to.x)),
					   static_cast<uint8_t>(std::max(from.y, to.y)),
					   static_cast<uint8_t>(std::max(from.z, to.z))};

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

		return model;
	}

} // anonymous namespace

// ---------------------------------------------------------------------------
// onion::voxel::BlockRegistry implementation
// ---------------------------------------------------------------------------
namespace onion::voxel
{
	BlockRegistry::BlockRegistry(std::shared_ptr<TextureAtlas> atlas) : m_Atlas(atlas) {}

	// ---- Direct texture-array registration (used by special-case blocks) ----
	void BlockRegistry::RegisterModel(BlockId id, const std::array<TextureInfo, 6>& textures, Model textureModel)
	{
		for (const auto& texture : textures)
			if (!texture.name.empty())
				m_AllTextureNames.insert(texture.name);

		uint8_t variantIndex = m_VariantCounters[id]++;

		PreRegistration reg;
		reg.id = id;
		reg.variantIndex = variantIndex;
		reg.textures = textures;
		reg.textureModel = textureModel;
		m_Registrations.emplace_back(reg);
	}

	void BlockRegistry::PreSetOverlay(BlockId id, Face face, const TextureInfo& texture)
	{
		m_AllTextureNames.insert(texture.name);
		m_RegistrationsOverlays.emplace_back(id, face, texture);
	}

	// ---- Register one VariantModel for a block ----
	void BlockRegistry::RegisterVariant(BlockId id, const VariantModel& variant)
	{
		// Strip path prefix to get bare model filename
		const std::string& path = variant.ModelPath;
		auto slashPos = path.find_last_of('/');
		std::string model = (slashPos != std::string::npos) ? path.substr(slashPos + 1) : path;

		BlockModel blockModel = BlockModel::FromFile(model + ".json");

		// ----- Special-case magic -----
		if (id == BlockId::Water)
		{
			BlockModel stoneModel = BlockModel::FromFile("stone.json");
			stoneModel.ModelTextures["all"] = blockModel.ModelTextures["particle"];
			blockModel = stoneModel;
			for (auto& elem : blockModel.Elements)
				for (auto& [fn, face] : elem.Faces)
					face.TintIndex = 1; // Water tint
		}
		else if (id == BlockId::Lava)
		{
			BlockModel stoneModel = BlockModel::FromFile("stone.json");
			stoneModel.ModelTextures["all"] = blockModel.ModelTextures["particle"];
			blockModel = stoneModel;
		}

		Model textureModel = Model::Block;
		if (!blockModel.ModelTextures["cross"].empty())
		{
			textureModel = Model::Cross;
			BlockModel stoneModel = BlockModel::FromFile("stone.json");
			stoneModel.ModelTextures["all"] = blockModel.ModelTextures["cross"];
			blockModel = stoneModel;
			if (id == BlockId::ShortGrass)
				for (auto& elem : blockModel.Elements)
					for (auto& [fn, face] : elem.Faces)
						face.TintIndex = 0; // Grass tint
		}

		if (blockModel.Elements.empty())
		{
			std::cout << "Warning: BlockModel for block ID " << static_cast<uint16_t>(id) << " (" << model
					  << ") has no elements." << std::endl;
			blockModel = BlockModel::FromFile("bubble_coral_block.json");
		}

		// ----- Bake blockstate rotation into element geometry -----
		const int stepsX = ((variant.RotationX / 90) % 4 + 4) % 4;
		const int stepsY = ((variant.RotationY / 90) % 4 + 4) % 4;
		if (stepsX != 0 || stepsY != 0)
			blockModel = ApplyModelRotation(std::move(blockModel), stepsX, stepsY);

		// ----- Convert elements to TextureInfo -----
		std::array<TextureInfo, 6> baseTextures{};
		bool baseInitialized = false;

		for (const auto& elem : blockModel.Elements)
		{
			bool isOverlay = IsOverlayElem(elem);

			for (const auto& [faceName, face] : elem.Faces)
			{
				Face f = ToFace(faceName);

				std::string resolved = ResolveTexture(face.Texture, blockModel.ModelTextures);
				if (resolved.empty())
					throw std::runtime_error("Failed to resolve texture reference: " + face.Texture);

				Tint tint = Tint::None;
				if (face.TintIndex.has_value())
				{
					if (face.TintIndex.value() == 0)
						tint = Tint::Grass;
					else if (face.TintIndex.value() == 1)
						tint = Tint::Water;
				}

				if (!isOverlay)
				{
					glm::u8vec3 from = {elem.From[0], elem.From[1], elem.From[2]};
					glm::u8vec3 to = {elem.To[0], elem.To[1], elem.To[2]};
					baseTextures[(int) f] = TextureInfo{resolved, tint, from, to, face.UV};
					baseInitialized = true;
				}
				else
				{
					PreSetOverlay(id, f, TextureInfo{resolved, tint});
				}
			}
		}

		if (baseInitialized)
			RegisterModel(id, baseTextures, textureModel);
	}

	// ---- Register all variants parsed from a blockstate JSON ----
	void BlockRegistry::RegisterModel(BlockId id, const std::string& blockstate)
	{
		BlockStateJson blockStateJson = BlockStateJson::FromFile(blockstate);

		BlockIds::RegisterBlockIdName(id, ToBlockName(blockstate));

		// Store conditions for ResolveVariantIndex
		m_VariantConditions[id] = blockStateJson.Variants;

		for (const auto& variant : blockStateJson.Variants)
		{
			if (variant.Models.empty())
				continue;
			// Register the first model as the representative for this variant.
			// Weighted random selection can be layered on top later.
			RegisterVariant(id, variant.Models[0]);
		}
	}

	// ---- Finalize: resolve TextureIDs from atlas ----
	void BlockRegistry::Register(BlockId id,
								 uint8_t variantIndex,
								 const std::array<TextureInfo, 6>& textures,
								 Model textureModel)
	{
		BlockTextures tex;
		tex.rotationType = BlockState::GetRotationType(id);
		tex.textureModel = textureModel;

		for (size_t i = 0; i < 6; i++)
		{
			const std::string& textureName = textures[i].name;
			if (textureName.empty())
				continue;

			tex.faces[i].texture = m_Atlas->GetTextureID(textureName);
			tex.faces[i].tintType = textures[i].tintType;
			tex.faces[i].textureType = m_Atlas->GetTextureTransparency(textureName);
			tex.faces[i].from = textures[i].from;
			tex.faces[i].to = textures[i].to;
			tex.faces[i].uv = textures[i].uv;

			m_AllTextureNames.insert(textureName);
		}

		auto& variants = m_Blocks[id];
		// Grow the vector to fit this variant index
		if (variantIndex >= variants.size())
			variants.resize(variantIndex + 1);
		variants[variantIndex] = tex;
	}

	void BlockRegistry::SetOverlay(BlockId id, Face face, const TextureInfo& texture)
	{
		auto it = m_Blocks.find(id);
		if (it == m_Blocks.end() || it->second.empty())
		{
			throw std::runtime_error("BlockRegistry::SetOverlay: Block ID not found: " +
									 std::to_string(static_cast<uint16_t>(id)));
		}

		// Apply overlay to all variants of this block
		for (auto& blockTex : it->second)
		{
			blockTex.overlay[static_cast<size_t>(face)].texture = m_Atlas->GetTextureID(texture.name);
			blockTex.overlay[static_cast<size_t>(face)].tintType = texture.tintType;
			blockTex.overlay[static_cast<size_t>(face)].textureType = m_Atlas->GetTextureTransparency(texture.name);
		}

		m_AllTextureNames.insert(texture.name);
	}

	void BlockRegistry::Initialize()
	{
		ReloadTextures();
	}

	void BlockRegistry::ReloadTextures()
	{
		ReloadModels();

		m_Blocks.clear();

		for (const auto& registration : m_Registrations)
			Register(registration.id, registration.variantIndex, registration.textures, registration.textureModel);

		for (const auto& overlay : m_RegistrationsOverlays)
			SetOverlay(overlay.id, overlay.face, overlay.texture);
	}

	const std::unordered_set<std::string>& BlockRegistry::GetAllTextureNames() const
	{
		return m_AllTextureNames;
	}

	const BlockTextures& BlockRegistry::Get(BlockId id) const
	{
		return Get(id, 0);
	}

	const BlockTextures& BlockRegistry::Get(BlockId id, uint8_t variantIndex) const
	{
		auto it = m_Blocks.find(id);
		if (it == m_Blocks.end() || it->second.empty())
		{
			throw std::runtime_error("BlockRegistry::Get: Block ID not found: " +
									 std::to_string(static_cast<uint16_t>(id)));
		}

		// Clamp to valid range — prevents crashes from stale variant indices
		const auto& variants = it->second;
		const size_t idx = std::min(static_cast<size_t>(variantIndex), variants.size() - 1);
		return variants[idx];
	}

	size_t BlockRegistry::GetVariantCount(BlockId id) const
	{
		auto it = m_Blocks.find(id);
		if (it == m_Blocks.end())
			return 0;
		return it->second.size();
	}

	uint8_t BlockRegistry::ResolveVariantIndex(BlockId id, const std::map<std::string, std::string>& properties) const
	{
		auto it = m_VariantConditions.find(id);
		if (it == m_VariantConditions.end())
			return 0;

		const auto& variants = it->second;
		for (size_t i = 0; i < variants.size(); i++)
		{
			const auto& conditions = variants[i].Conditions;

			// Empty conditions = matches-all (single-variant blocks like stone)
			if (conditions.empty())
				return static_cast<uint8_t>(i);

			// Check every condition in the variant against the provided properties
			bool match = true;
			for (const auto& [key, val] : conditions)
			{
				auto pit = properties.find(key);
				if (pit == properties.end() || pit->second != val)
				{
					match = false;
					break;
				}
			}
			if (match)
				return static_cast<uint8_t>(i);
		}

		return 0; // Default fallback
	}

	void BlockRegistry::ReloadModels()
	{
		BlockModel::ClearCache();

		m_AllTextureNames.clear();
		m_Registrations.clear();
		m_RegistrationsOverlays.clear();
		m_VariantCounters.clear();
		m_VariantConditions.clear();

		RegisterModel(BlockId::AcaciaLeaves, "acacia_leaves.json");
		RegisterModel(BlockId::AcaciaLog, "acacia_log.json");
		RegisterModel(BlockId::AcaciaPlanks, "acacia_planks.json");
		RegisterModel(BlockId::AcaciaSapling, "acacia_sapling.json");
		RegisterModel(BlockId::AcaciaWood, "acacia_wood.json");
		RegisterModel(BlockId::Allium, "allium.json");
		RegisterModel(BlockId::AmethystBlock, "amethyst_block.json");
		RegisterModel(BlockId::AncientDebris, "ancient_debris.json");
		RegisterModel(BlockId::Andesite, "andesite.json");
		RegisterModel(BlockId::Azalea, "azalea.json");
		RegisterModel(BlockId::AzaleaLeaves, "azalea_leaves.json");
		RegisterModel(BlockId::AzureBluet, "azure_bluet.json");
		RegisterModel(BlockId::BambooBlock, "bamboo_block.json");
		RegisterModel(BlockId::BambooMosaic, "bamboo_mosaic.json");
		RegisterModel(BlockId::BambooPlanks, "bamboo_planks.json");
		RegisterModel(BlockId::BambooSapling, "bamboo_sapling.json");
		RegisterModel(BlockId::Barrier, "barrier.json");
		RegisterModel(BlockId::Basalt, "basalt.json");
		RegisterModel(BlockId::Beacon, "beacon.json");
		RegisterModel(BlockId::Bedrock, "bedrock.json");
		RegisterModel(BlockId::BirchLeaves, "birch_leaves.json");
		RegisterModel(BlockId::BirchLog, "birch_log.json");
		RegisterModel(BlockId::BirchPlanks, "birch_planks.json");
		RegisterModel(BlockId::BirchSapling, "birch_sapling.json");
		RegisterModel(BlockId::BirchWood, "birch_wood.json");
		RegisterModel(BlockId::Blackstone, "blackstone.json");
		RegisterModel(BlockId::BlackCarpet, "black_carpet.json");
		RegisterModel(BlockId::BlackConcrete, "black_concrete.json");
		RegisterModel(BlockId::BlackConcretePowder, "black_concrete_powder.json");
		RegisterModel(BlockId::BlackStainedGlass, "black_stained_glass.json");
		RegisterModel(BlockId::BlackTerracotta, "black_terracotta.json");
		RegisterModel(BlockId::BlackWool, "black_wool.json");
		RegisterModel(BlockId::BlueCarpet, "blue_carpet.json");
		RegisterModel(BlockId::BlueConcrete, "blue_concrete.json");
		RegisterModel(BlockId::BlueConcretePowder, "blue_concrete_powder.json");
		RegisterModel(BlockId::BlueIce, "blue_ice.json");
		RegisterModel(BlockId::BlueOrchid, "blue_orchid.json");
		RegisterModel(BlockId::BlueStainedGlass, "blue_stained_glass.json");
		RegisterModel(BlockId::BlueTerracotta, "blue_terracotta.json");
		RegisterModel(BlockId::BlueWool, "blue_wool.json");
		RegisterModel(BlockId::BoneBlock, "bone_block.json");
		RegisterModel(BlockId::Bookshelf, "bookshelf.json");
		RegisterModel(BlockId::BrainCoral, "brain_coral.json");
		RegisterModel(BlockId::BrainCoralBlock, "brain_coral_block.json");
		RegisterModel(BlockId::Bricks, "bricks.json");
		RegisterModel(BlockId::BrownCarpet, "brown_carpet.json");
		RegisterModel(BlockId::BrownConcrete, "brown_concrete.json");
		RegisterModel(BlockId::BrownConcretePowder, "brown_concrete_powder.json");
		RegisterModel(BlockId::BrownMushroom, "brown_mushroom.json");
		RegisterModel(BlockId::BrownStainedGlass, "brown_stained_glass.json");
		RegisterModel(BlockId::BrownTerracotta, "brown_terracotta.json");
		RegisterModel(BlockId::BrownWool, "brown_wool.json");
		RegisterModel(BlockId::BuddingAmethyst, "budding_amethyst.json");
		RegisterModel(BlockId::Bush, "bush.json");
		RegisterModel(BlockId::Cactus, "cactus.json");
		RegisterModel(BlockId::CactusFlower, "cactus_flower.json");
		RegisterModel(BlockId::Calcite, "calcite.json");
		RegisterModel(BlockId::CartographyTable, "cartography_table.json");
		RegisterModel(BlockId::Cauldron, "cauldron.json");
		RegisterModel(BlockId::CaveAir, "cave_air.json");
		RegisterModel(BlockId::CherryLeaves, "cherry_leaves.json");
		RegisterModel(BlockId::CherryLog, "cherry_log.json");
		RegisterModel(BlockId::CherryPlanks, "cherry_planks.json");
		RegisterModel(BlockId::CherrySapling, "cherry_sapling.json");
		RegisterModel(BlockId::CherryWood, "cherry_wood.json");
		RegisterModel(BlockId::ChiseledCopper, "chiseled_copper.json");
		RegisterModel(BlockId::ChiseledDeepslate, "chiseled_deepslate.json");
		RegisterModel(BlockId::ChiseledNetherBricks, "chiseled_nether_bricks.json");
		RegisterModel(BlockId::ChiseledPolishedBlackstone, "chiseled_polished_blackstone.json");
		RegisterModel(BlockId::ChiseledQuartzBlock, "chiseled_quartz_block.json");
		RegisterModel(BlockId::ChiseledRedSandstone, "chiseled_red_sandstone.json");
		RegisterModel(BlockId::ChiseledResinBricks, "chiseled_resin_bricks.json");
		RegisterModel(BlockId::ChiseledSandstone, "chiseled_sandstone.json");
		RegisterModel(BlockId::ChiseledStoneBricks, "chiseled_stone_bricks.json");
		RegisterModel(BlockId::ChiseledTuff, "chiseled_tuff.json");
		RegisterModel(BlockId::ChiseledTuffBricks, "chiseled_tuff_bricks.json");
		RegisterModel(BlockId::Clay, "clay.json");
		RegisterModel(BlockId::ClosedEyeblossom, "closed_eyeblossom.json");
		RegisterModel(BlockId::CoalBlock, "coal_block.json");
		RegisterModel(BlockId::CoalOre, "coal_ore.json");
		RegisterModel(BlockId::CoarseDirt, "coarse_dirt.json");
		RegisterModel(BlockId::CobbledDeepslate, "cobbled_deepslate.json");
		RegisterModel(BlockId::Cobblestone, "cobblestone.json");
		RegisterModel(BlockId::Cobweb, "cobweb.json");
		RegisterModel(BlockId::CopperBlock, "copper_block.json");
		RegisterModel(BlockId::CopperChain, "copper_chain.json");
		RegisterModel(BlockId::CopperGrate, "copper_grate.json");
		RegisterModel(BlockId::CopperOre, "copper_ore.json");
		RegisterModel(BlockId::CopperTorch, "copper_torch.json");
		RegisterModel(BlockId::Cornflower, "cornflower.json");
		RegisterModel(BlockId::CrackedDeepslateBricks, "cracked_deepslate_bricks.json");
		RegisterModel(BlockId::CrackedDeepslateTiles, "cracked_deepslate_tiles.json");
		RegisterModel(BlockId::CrackedNetherBricks, "cracked_nether_bricks.json");
		RegisterModel(BlockId::CrackedPolishedBlackstoneBricks, "cracked_polished_blackstone_bricks.json");
		RegisterModel(BlockId::CrackedStoneBricks, "cracked_stone_bricks.json");
		RegisterModel(BlockId::CraftingTable, "crafting_table.json");
		RegisterModel(BlockId::CrimsonFungus, "crimson_fungus.json");
		RegisterModel(BlockId::CrimsonHyphae, "crimson_hyphae.json");
		RegisterModel(BlockId::CrimsonNylium, "crimson_nylium.json");
		RegisterModel(BlockId::CrimsonPlanks, "crimson_planks.json");
		RegisterModel(BlockId::CrimsonRoots, "crimson_roots.json");
		RegisterModel(BlockId::CrimsonStem, "crimson_stem.json");
		RegisterModel(BlockId::CryingObsidian, "crying_obsidian.json");
		RegisterModel(BlockId::CutCopper, "cut_copper.json");
		RegisterModel(BlockId::CutRedSandstone, "cut_red_sandstone.json");
		RegisterModel(BlockId::CutSandstone, "cut_sandstone.json");
		RegisterModel(BlockId::CyanCarpet, "cyan_carpet.json");
		RegisterModel(BlockId::CyanConcrete, "cyan_concrete.json");
		RegisterModel(BlockId::CyanConcretePowder, "cyan_concrete_powder.json");
		RegisterModel(BlockId::CyanStainedGlass, "cyan_stained_glass.json");
		RegisterModel(BlockId::CyanTerracotta, "cyan_terracotta.json");
		RegisterModel(BlockId::CyanWool, "cyan_wool.json");
		RegisterModel(BlockId::Dandelion, "dandelion.json");
		RegisterModel(BlockId::DarkOakLeaves, "dark_oak_leaves.json");
		RegisterModel(BlockId::DarkOakLog, "dark_oak_log.json");
		RegisterModel(BlockId::DarkOakPlanks, "dark_oak_planks.json");
		RegisterModel(BlockId::DarkOakSapling, "dark_oak_sapling.json");
		RegisterModel(BlockId::DarkOakWood, "dark_oak_wood.json");
		RegisterModel(BlockId::DarkPrismarine, "dark_prismarine.json");
		RegisterModel(BlockId::DeadBrainCoral, "dead_brain_coral.json");
		RegisterModel(BlockId::DeadBrainCoralBlock, "dead_brain_coral_block.json");
		RegisterModel(BlockId::DeadBush, "dead_bush.json");
		RegisterModel(BlockId::DeadFireCoral, "dead_fire_coral.json");
		RegisterModel(BlockId::DeadFireCoralBlock, "dead_fire_coral_block.json");
		RegisterModel(BlockId::DeadHornCoral, "dead_horn_coral.json");
		RegisterModel(BlockId::DeadHornCoralBlock, "dead_horn_coral_block.json");
		RegisterModel(BlockId::DeadTubeCoral, "dead_tube_coral.json");
		RegisterModel(BlockId::DeadTubeCoralBlock, "dead_tube_coral_block.json");
		RegisterModel(BlockId::DecoratedPot, "decorated_pot.json");
		RegisterModel(BlockId::Deepslate, "deepslate.json");
		RegisterModel(BlockId::DeepslateBricks, "deepslate_bricks.json");
		RegisterModel(BlockId::DeepslateCoalOre, "deepslate_coal_ore.json");
		RegisterModel(BlockId::DeepslateCopperOre, "deepslate_copper_ore.json");
		RegisterModel(BlockId::DeepslateDiamondOre, "deepslate_diamond_ore.json");
		RegisterModel(BlockId::DeepslateEmeraldOre, "deepslate_emerald_ore.json");
		RegisterModel(BlockId::DeepslateGoldOre, "deepslate_gold_ore.json");
		RegisterModel(BlockId::DeepslateIronOre, "deepslate_iron_ore.json");
		RegisterModel(BlockId::DeepslateLapisOre, "deepslate_lapis_ore.json");
		RegisterModel(BlockId::DeepslateRedstoneOre, "deepslate_redstone_ore.json");
		RegisterModel(BlockId::DeepslateTiles, "deepslate_tiles.json");
		RegisterModel(BlockId::DiamondBlock, "diamond_block.json");
		RegisterModel(BlockId::DiamondOre, "diamond_ore.json");
		RegisterModel(BlockId::Diorite, "diorite.json");
		RegisterModel(BlockId::Dirt, "dirt.json");
		RegisterModel(BlockId::DirtPath, "dirt_path.json");
		RegisterModel(BlockId::DragonEgg, "dragon_egg.json");
		RegisterModel(BlockId::DriedKelpBlock, "dried_kelp_block.json");
		RegisterModel(BlockId::DripstoneBlock, "dripstone_block.json");
		RegisterModel(BlockId::EmeraldBlock, "emerald_block.json");
		RegisterModel(BlockId::EmeraldOre, "emerald_ore.json");
		RegisterModel(BlockId::EnchantingTable, "enchanting_table.json");
		RegisterModel(BlockId::EndStone, "end_stone.json");
		RegisterModel(BlockId::EndStoneBricks, "end_stone_bricks.json");
		RegisterModel(BlockId::ExposedChiseledCopper, "exposed_chiseled_copper.json");
		RegisterModel(BlockId::ExposedCopper, "exposed_copper.json");
		RegisterModel(BlockId::ExposedCopperChain, "exposed_copper_chain.json");
		RegisterModel(BlockId::ExposedCopperGrate, "exposed_copper_grate.json");
		RegisterModel(BlockId::ExposedCutCopper, "exposed_cut_copper.json");
		RegisterModel(BlockId::Fern, "fern.json");
		RegisterModel(BlockId::FireflyBush, "firefly_bush.json");
		RegisterModel(BlockId::FireCoral, "fire_coral.json");
		RegisterModel(BlockId::FireCoralBlock, "fire_coral_block.json");
		RegisterModel(BlockId::FletchingTable, "fletching_table.json");
		RegisterModel(BlockId::FloweringAzalea, "flowering_azalea.json");
		RegisterModel(BlockId::FloweringAzaleaLeaves, "flowering_azalea_leaves.json");
		RegisterModel(BlockId::FlowerPot, "flower_pot.json");
		RegisterModel(BlockId::GildedBlackstone, "gilded_blackstone.json");
		RegisterModel(BlockId::Glass, "glass.json");
		RegisterModel(BlockId::Glowstone, "glowstone.json");
		RegisterModel(BlockId::GoldBlock, "gold_block.json");
		RegisterModel(BlockId::GoldOre, "gold_ore.json");
		RegisterModel(BlockId::Granite, "granite.json");
		RegisterModel(BlockId::GrassBlock, "grass_block.json");
		RegisterModel(BlockId::Gravel, "gravel.json");
		RegisterModel(BlockId::GrayCarpet, "gray_carpet.json");
		RegisterModel(BlockId::GrayConcrete, "gray_concrete.json");
		RegisterModel(BlockId::GrayConcretePowder, "gray_concrete_powder.json");
		RegisterModel(BlockId::GrayStainedGlass, "gray_stained_glass.json");
		RegisterModel(BlockId::GrayTerracotta, "gray_terracotta.json");
		RegisterModel(BlockId::GrayWool, "gray_wool.json");
		RegisterModel(BlockId::GreenCarpet, "green_carpet.json");
		RegisterModel(BlockId::GreenConcrete, "green_concrete.json");
		RegisterModel(BlockId::GreenConcretePowder, "green_concrete_powder.json");
		RegisterModel(BlockId::GreenStainedGlass, "green_stained_glass.json");
		RegisterModel(BlockId::GreenTerracotta, "green_terracotta.json");
		RegisterModel(BlockId::GreenWool, "green_wool.json");
		RegisterModel(BlockId::HangingRoots, "hanging_roots.json");
		RegisterModel(BlockId::HayBlock, "hay_block.json");
		RegisterModel(BlockId::HeavyCore, "heavy_core.json");
		RegisterModel(BlockId::HoneycombBlock, "honeycomb_block.json");
		RegisterModel(BlockId::HoneyBlock, "honey_block.json");
		RegisterModel(BlockId::HornCoral, "horn_coral.json");
		RegisterModel(BlockId::HornCoralBlock, "horn_coral_block.json");
		RegisterModel(BlockId::Ice, "ice.json");
		RegisterModel(BlockId::InfestedChiseledStoneBricks, "infested_chiseled_stone_bricks.json");
		RegisterModel(BlockId::InfestedCobblestone, "infested_cobblestone.json");
		RegisterModel(BlockId::InfestedCrackedStoneBricks, "infested_cracked_stone_bricks.json");
		RegisterModel(BlockId::InfestedDeepslate, "infested_deepslate.json");
		RegisterModel(BlockId::InfestedMossyStoneBricks, "infested_mossy_stone_bricks.json");
		RegisterModel(BlockId::InfestedStone, "infested_stone.json");
		RegisterModel(BlockId::InfestedStoneBricks, "infested_stone_bricks.json");
		RegisterModel(BlockId::IronBlock, "iron_block.json");
		RegisterModel(BlockId::IronChain, "iron_chain.json");
		RegisterModel(BlockId::IronOre, "iron_ore.json");
		RegisterModel(BlockId::Jukebox, "jukebox.json");
		RegisterModel(BlockId::JungleLeaves, "jungle_leaves.json");
		RegisterModel(BlockId::JungleLog, "jungle_log.json");
		RegisterModel(BlockId::JunglePlanks, "jungle_planks.json");
		RegisterModel(BlockId::JungleSapling, "jungle_sapling.json");
		RegisterModel(BlockId::JungleWood, "jungle_wood.json");
		RegisterModel(BlockId::Kelp, "kelp.json");
		RegisterModel(BlockId::KelpPlant, "kelp_plant.json");
		RegisterModel(BlockId::LapisBlock, "lapis_block.json");
		RegisterModel(BlockId::LapisOre, "lapis_ore.json");
		RegisterModel(BlockId::Lava, "lava.json");
		RegisterModel(BlockId::LavaCauldron, "lava_cauldron.json");
		RegisterModel(BlockId::LightBlueCarpet, "light_blue_carpet.json");
		RegisterModel(BlockId::LightBlueConcrete, "light_blue_concrete.json");
		RegisterModel(BlockId::LightBlueConcretePowder, "light_blue_concrete_powder.json");
		RegisterModel(BlockId::LightBlueStainedGlass, "light_blue_stained_glass.json");
		RegisterModel(BlockId::LightBlueTerracotta, "light_blue_terracotta.json");
		RegisterModel(BlockId::LightBlueWool, "light_blue_wool.json");
		RegisterModel(BlockId::LightGrayCarpet, "light_gray_carpet.json");
		RegisterModel(BlockId::LightGrayConcrete, "light_gray_concrete.json");
		RegisterModel(BlockId::LightGrayConcretePowder, "light_gray_concrete_powder.json");
		RegisterModel(BlockId::LightGrayStainedGlass, "light_gray_stained_glass.json");
		RegisterModel(BlockId::LightGrayTerracotta, "light_gray_terracotta.json");
		RegisterModel(BlockId::LightGrayWool, "light_gray_wool.json");
		RegisterModel(BlockId::LilyOfTheValley, "lily_of_the_valley.json");
		RegisterModel(BlockId::LilyPad, "lily_pad.json");
		RegisterModel(BlockId::LimeCarpet, "lime_carpet.json");
		RegisterModel(BlockId::LimeConcrete, "lime_concrete.json");
		RegisterModel(BlockId::LimeConcretePowder, "lime_concrete_powder.json");
		RegisterModel(BlockId::LimeStainedGlass, "lime_stained_glass.json");
		RegisterModel(BlockId::LimeTerracotta, "lime_terracotta.json");
		RegisterModel(BlockId::LimeWool, "lime_wool.json");
		RegisterModel(BlockId::Lodestone, "lodestone.json");
		RegisterModel(BlockId::MagentaCarpet, "magenta_carpet.json");
		RegisterModel(BlockId::MagentaConcrete, "magenta_concrete.json");
		RegisterModel(BlockId::MagentaConcretePowder, "magenta_concrete_powder.json");
		RegisterModel(BlockId::MagentaStainedGlass, "magenta_stained_glass.json");
		RegisterModel(BlockId::MagentaTerracotta, "magenta_terracotta.json");
		RegisterModel(BlockId::MagentaWool, "magenta_wool.json");
		RegisterModel(BlockId::MagmaBlock, "magma_block.json");
		RegisterModel(BlockId::MangroveLeaves, "mangrove_leaves.json");
		RegisterModel(BlockId::MangroveLog, "mangrove_log.json");
		RegisterModel(BlockId::MangrovePlanks, "mangrove_planks.json");
		RegisterModel(BlockId::MangroveRoots, "mangrove_roots.json");
		RegisterModel(BlockId::MangroveWood, "mangrove_wood.json");
		RegisterModel(BlockId::Melon, "melon.json");
		RegisterModel(BlockId::MossyCobblestone, "mossy_cobblestone.json");
		RegisterModel(BlockId::MossyStoneBricks, "mossy_stone_bricks.json");
		RegisterModel(BlockId::MossBlock, "moss_block.json");
		RegisterModel(BlockId::MossCarpet, "moss_carpet.json");
		RegisterModel(BlockId::MovingPiston, "moving_piston.json");
		RegisterModel(BlockId::Mud, "mud.json");
		RegisterModel(BlockId::MuddyMangroveRoots, "muddy_mangrove_roots.json");
		RegisterModel(BlockId::MudBricks, "mud_bricks.json");
		RegisterModel(BlockId::Mycelium, "mycelium.json");
		RegisterModel(BlockId::NetheriteBlock, "netherite_block.json");
		RegisterModel(BlockId::Netherrack, "netherrack.json");
		RegisterModel(BlockId::NetherBricks, "nether_bricks.json");
		RegisterModel(BlockId::NetherGoldOre, "nether_gold_ore.json");
		RegisterModel(BlockId::NetherPortal, "nether_portal.json");
		RegisterModel(BlockId::NetherQuartzOre, "nether_quartz_ore.json");
		RegisterModel(BlockId::NetherSprouts, "nether_sprouts.json");
		RegisterModel(BlockId::NetherWartBlock, "nether_wart_block.json");
		RegisterModel(BlockId::NoteBlock, "note_block.json");
		RegisterModel(BlockId::OakLeaves, "oak_leaves.json");
		RegisterModel(BlockId::OakLog, "oak_log.json");
		RegisterModel(BlockId::OakPlanks, "oak_planks.json");
		RegisterModel(BlockId::OakSapling, "oak_sapling.json");
		RegisterModel(BlockId::OakWood, "oak_wood.json");
		RegisterModel(BlockId::Obsidian, "obsidian.json");
		RegisterModel(BlockId::OchreFroglight, "ochre_froglight.json");
		RegisterModel(BlockId::OpenEyeblossom, "open_eyeblossom.json");
		RegisterModel(BlockId::OrangeCarpet, "orange_carpet.json");
		RegisterModel(BlockId::OrangeConcrete, "orange_concrete.json");
		RegisterModel(BlockId::OrangeConcretePowder, "orange_concrete_powder.json");
		RegisterModel(BlockId::OrangeStainedGlass, "orange_stained_glass.json");
		RegisterModel(BlockId::OrangeTerracotta, "orange_terracotta.json");
		RegisterModel(BlockId::OrangeTulip, "orange_tulip.json");
		RegisterModel(BlockId::OrangeWool, "orange_wool.json");
		RegisterModel(BlockId::OxeyeDaisy, "oxeye_daisy.json");
		RegisterModel(BlockId::OxidizedChiseledCopper, "oxidized_chiseled_copper.json");
		RegisterModel(BlockId::OxidizedCopper, "oxidized_copper.json");
		RegisterModel(BlockId::OxidizedCopperChain, "oxidized_copper_chain.json");
		RegisterModel(BlockId::OxidizedCopperGrate, "oxidized_copper_grate.json");
		RegisterModel(BlockId::OxidizedCutCopper, "oxidized_cut_copper.json");
		RegisterModel(BlockId::PackedIce, "packed_ice.json");
		RegisterModel(BlockId::PackedMud, "packed_mud.json");
		RegisterModel(BlockId::PaleMossBlock, "pale_moss_block.json");
		RegisterModel(BlockId::PaleOakLeaves, "pale_oak_leaves.json");
		RegisterModel(BlockId::PaleOakLog, "pale_oak_log.json");
		RegisterModel(BlockId::PaleOakPlanks, "pale_oak_planks.json");
		RegisterModel(BlockId::PaleOakSapling, "pale_oak_sapling.json");
		RegisterModel(BlockId::PaleOakWood, "pale_oak_wood.json");
		RegisterModel(BlockId::PearlescentFroglight, "pearlescent_froglight.json");
		RegisterModel(BlockId::PinkCarpet, "pink_carpet.json");
		RegisterModel(BlockId::PinkConcrete, "pink_concrete.json");
		RegisterModel(BlockId::PinkConcretePowder, "pink_concrete_powder.json");
		RegisterModel(BlockId::PinkStainedGlass, "pink_stained_glass.json");
		RegisterModel(BlockId::PinkTerracotta, "pink_terracotta.json");
		RegisterModel(BlockId::PinkTulip, "pink_tulip.json");
		RegisterModel(BlockId::PinkWool, "pink_wool.json");
		RegisterModel(BlockId::Podzol, "podzol.json");
		RegisterModel(BlockId::PolishedAndesite, "polished_andesite.json");
		RegisterModel(BlockId::PolishedBasalt, "polished_basalt.json");
		RegisterModel(BlockId::PolishedBlackstone, "polished_blackstone.json");
		RegisterModel(BlockId::PolishedBlackstoneBricks, "polished_blackstone_bricks.json");
		RegisterModel(BlockId::PolishedDeepslate, "polished_deepslate.json");
		RegisterModel(BlockId::PolishedDiorite, "polished_diorite.json");
		RegisterModel(BlockId::PolishedGranite, "polished_granite.json");
		RegisterModel(BlockId::PolishedTuff, "polished_tuff.json");
		RegisterModel(BlockId::Poppy, "poppy.json");
		RegisterModel(BlockId::PottedAcaciaSapling, "potted_acacia_sapling.json");
		RegisterModel(BlockId::PottedAllium, "potted_allium.json");
		RegisterModel(BlockId::PottedAzaleaBush, "potted_azalea_bush.json");
		RegisterModel(BlockId::PottedAzureBluet, "potted_azure_bluet.json");
		RegisterModel(BlockId::PottedBamboo, "potted_bamboo.json");
		RegisterModel(BlockId::PottedBirchSapling, "potted_birch_sapling.json");
		RegisterModel(BlockId::PottedBlueOrchid, "potted_blue_orchid.json");
		RegisterModel(BlockId::PottedBrownMushroom, "potted_brown_mushroom.json");
		RegisterModel(BlockId::PottedCactus, "potted_cactus.json");
		RegisterModel(BlockId::PottedCherrySapling, "potted_cherry_sapling.json");
		RegisterModel(BlockId::PottedClosedEyeblossom, "potted_closed_eyeblossom.json");
		RegisterModel(BlockId::PottedCornflower, "potted_cornflower.json");
		RegisterModel(BlockId::PottedCrimsonFungus, "potted_crimson_fungus.json");
		RegisterModel(BlockId::PottedCrimsonRoots, "potted_crimson_roots.json");
		RegisterModel(BlockId::PottedDandelion, "potted_dandelion.json");
		RegisterModel(BlockId::PottedDarkOakSapling, "potted_dark_oak_sapling.json");
		RegisterModel(BlockId::PottedDeadBush, "potted_dead_bush.json");
		RegisterModel(BlockId::PottedFern, "potted_fern.json");
		RegisterModel(BlockId::PottedFloweringAzaleaBush, "potted_flowering_azalea_bush.json");
		RegisterModel(BlockId::PottedJungleSapling, "potted_jungle_sapling.json");
		RegisterModel(BlockId::PottedLilyOfTheValley, "potted_lily_of_the_valley.json");
		RegisterModel(BlockId::PottedMangrovePropagule, "potted_mangrove_propagule.json");
		RegisterModel(BlockId::PottedOakSapling, "potted_oak_sapling.json");
		RegisterModel(BlockId::PottedOpenEyeblossom, "potted_open_eyeblossom.json");
		RegisterModel(BlockId::PottedOrangeTulip, "potted_orange_tulip.json");
		RegisterModel(BlockId::PottedOxeyeDaisy, "potted_oxeye_daisy.json");
		RegisterModel(BlockId::PottedPaleOakSapling, "potted_pale_oak_sapling.json");
		RegisterModel(BlockId::PottedPinkTulip, "potted_pink_tulip.json");
		RegisterModel(BlockId::PottedPoppy, "potted_poppy.json");
		RegisterModel(BlockId::PottedRedMushroom, "potted_red_mushroom.json");
		RegisterModel(BlockId::PottedRedTulip, "potted_red_tulip.json");
		RegisterModel(BlockId::PottedSpruceSapling, "potted_spruce_sapling.json");
		RegisterModel(BlockId::PottedTorchflower, "potted_torchflower.json");
		RegisterModel(BlockId::PottedWarpedFungus, "potted_warped_fungus.json");
		RegisterModel(BlockId::PottedWarpedRoots, "potted_warped_roots.json");
		RegisterModel(BlockId::PottedWhiteTulip, "potted_white_tulip.json");
		RegisterModel(BlockId::PottedWitherRose, "potted_wither_rose.json");
		RegisterModel(BlockId::PowderSnow, "powder_snow.json");
		RegisterModel(BlockId::Prismarine, "prismarine.json");
		RegisterModel(BlockId::PrismarineBricks, "prismarine_bricks.json");
		RegisterModel(BlockId::Pumpkin, "pumpkin.json");
		RegisterModel(BlockId::PurpleCarpet, "purple_carpet.json");
		RegisterModel(BlockId::PurpleConcrete, "purple_concrete.json");
		RegisterModel(BlockId::PurpleConcretePowder, "purple_concrete_powder.json");
		RegisterModel(BlockId::PurpleStainedGlass, "purple_stained_glass.json");
		RegisterModel(BlockId::PurpleTerracotta, "purple_terracotta.json");
		RegisterModel(BlockId::PurpleWool, "purple_wool.json");
		RegisterModel(BlockId::PurpurBlock, "purpur_block.json");
		RegisterModel(BlockId::PurpurPillar, "purpur_pillar.json");
		RegisterModel(BlockId::QuartzBlock, "quartz_block.json");
		RegisterModel(BlockId::QuartzBricks, "quartz_bricks.json");
		RegisterModel(BlockId::QuartzPillar, "quartz_pillar.json");
		RegisterModel(BlockId::RawCopperBlock, "raw_copper_block.json");
		RegisterModel(BlockId::RawGoldBlock, "raw_gold_block.json");
		RegisterModel(BlockId::RawIronBlock, "raw_iron_block.json");
		RegisterModel(BlockId::RedstoneBlock, "redstone_block.json");
		RegisterModel(BlockId::RedstoneOre, "redstone_ore.json");
		RegisterModel(BlockId::RedCarpet, "red_carpet.json");
		RegisterModel(BlockId::RedConcrete, "red_concrete.json");
		RegisterModel(BlockId::RedConcretePowder, "red_concrete_powder.json");
		RegisterModel(BlockId::RedMushroom, "red_mushroom.json");
		RegisterModel(BlockId::RedNetherBricks, "red_nether_bricks.json");
		RegisterModel(BlockId::RedSand, "red_sand.json");
		RegisterModel(BlockId::RedSandstone, "red_sandstone.json");
		RegisterModel(BlockId::RedStainedGlass, "red_stained_glass.json");
		RegisterModel(BlockId::RedTerracotta, "red_terracotta.json");
		RegisterModel(BlockId::RedTulip, "red_tulip.json");
		RegisterModel(BlockId::RedWool, "red_wool.json");
		RegisterModel(BlockId::ReinforcedDeepslate, "reinforced_deepslate.json");
		RegisterModel(BlockId::ResinBlock, "resin_block.json");
		RegisterModel(BlockId::ResinBricks, "resin_bricks.json");
		RegisterModel(BlockId::RootedDirt, "rooted_dirt.json");
		RegisterModel(BlockId::Sand, "sand.json");
		RegisterModel(BlockId::Sandstone, "sandstone.json");
		RegisterModel(BlockId::Sculk, "sculk.json");
		RegisterModel(BlockId::Seagrass, "seagrass.json");
		RegisterModel(BlockId::SeaLantern, "sea_lantern.json");
		RegisterModel(BlockId::ShortDryGrass, "short_dry_grass.json");
		RegisterModel(BlockId::ShortGrass, "short_grass.json");
		RegisterModel(BlockId::Shroomlight, "shroomlight.json");
		RegisterModel(BlockId::SkeletonSkull, "skeleton_skull.json");
		RegisterModel(BlockId::SkeletonWallSkull, "skeleton_wall_skull.json");
		RegisterModel(BlockId::SlimeBlock, "slime_block.json");
		RegisterModel(BlockId::SmithingTable, "smithing_table.json");
		RegisterModel(BlockId::SmoothBasalt, "smooth_basalt.json");
		RegisterModel(BlockId::SmoothQuartz, "smooth_quartz.json");
		RegisterModel(BlockId::SmoothRedSandstone, "smooth_red_sandstone.json");
		RegisterModel(BlockId::SmoothSandstone, "smooth_sandstone.json");
		RegisterModel(BlockId::SmoothStone, "smooth_stone.json");
		RegisterModel(BlockId::SnowBlock, "snow_block.json");
		RegisterModel(BlockId::SoulSand, "soul_sand.json");
		RegisterModel(BlockId::SoulSoil, "soul_soil.json");
		RegisterModel(BlockId::SoulTorch, "soul_torch.json");
		RegisterModel(BlockId::Spawner, "spawner.json");
		RegisterModel(BlockId::Sponge, "sponge.json");
		RegisterModel(BlockId::SporeBlossom, "spore_blossom.json");
		RegisterModel(BlockId::SpruceLeaves, "spruce_leaves.json");
		RegisterModel(BlockId::SpruceLog, "spruce_log.json");
		RegisterModel(BlockId::SprucePlanks, "spruce_planks.json");
		RegisterModel(BlockId::SpruceSapling, "spruce_sapling.json");
		RegisterModel(BlockId::SpruceWood, "spruce_wood.json");
		RegisterModel(BlockId::Stone, "stone.json");
		RegisterModel(BlockId::StoneBricks, "stone_bricks.json");
		RegisterModel(BlockId::StrippedAcaciaLog, "stripped_acacia_log.json");
		RegisterModel(BlockId::StrippedAcaciaWood, "stripped_acacia_wood.json");
		RegisterModel(BlockId::StrippedBambooBlock, "stripped_bamboo_block.json");
		RegisterModel(BlockId::StrippedBirchLog, "stripped_birch_log.json");
		RegisterModel(BlockId::StrippedBirchWood, "stripped_birch_wood.json");
		RegisterModel(BlockId::StrippedCherryLog, "stripped_cherry_log.json");
		RegisterModel(BlockId::StrippedCherryWood, "stripped_cherry_wood.json");
		RegisterModel(BlockId::StrippedCrimsonHyphae, "stripped_crimson_hyphae.json");
		RegisterModel(BlockId::StrippedCrimsonStem, "stripped_crimson_stem.json");
		RegisterModel(BlockId::StrippedDarkOakLog, "stripped_dark_oak_log.json");
		RegisterModel(BlockId::StrippedDarkOakWood, "stripped_dark_oak_wood.json");
		RegisterModel(BlockId::StrippedJungleLog, "stripped_jungle_log.json");
		RegisterModel(BlockId::StrippedJungleWood, "stripped_jungle_wood.json");
		RegisterModel(BlockId::StrippedMangroveLog, "stripped_mangrove_log.json");
		RegisterModel(BlockId::StrippedMangroveWood, "stripped_mangrove_wood.json");
		RegisterModel(BlockId::StrippedOakLog, "stripped_oak_log.json");
		RegisterModel(BlockId::StrippedOakWood, "stripped_oak_wood.json");
		RegisterModel(BlockId::StrippedPaleOakLog, "stripped_pale_oak_log.json");
		RegisterModel(BlockId::StrippedPaleOakWood, "stripped_pale_oak_wood.json");
		RegisterModel(BlockId::StrippedSpruceLog, "stripped_spruce_log.json");
		RegisterModel(BlockId::StrippedSpruceWood, "stripped_spruce_wood.json");
		RegisterModel(BlockId::StrippedWarpedHyphae, "stripped_warped_hyphae.json");
		RegisterModel(BlockId::StrippedWarpedStem, "stripped_warped_stem.json");
		RegisterModel(BlockId::StructureVoid, "structure_void.json");
		RegisterModel(BlockId::SugarCane, "sugar_cane.json");
		RegisterModel(BlockId::TallDryGrass, "tall_dry_grass.json");
		RegisterModel(BlockId::Target, "target.json");
		RegisterModel(BlockId::Terracotta, "terracotta.json");
		RegisterModel(BlockId::TestInstanceBlock, "test_instance_block.json");
		RegisterModel(BlockId::TintedGlass, "tinted_glass.json");
		RegisterModel(BlockId::Tnt, "tnt.json");
		RegisterModel(BlockId::Torch, "torch.json");
		RegisterModel(BlockId::Torchflower, "torchflower.json");
		RegisterModel(BlockId::TubeCoral, "tube_coral.json");
		RegisterModel(BlockId::TubeCoralBlock, "tube_coral_block.json");
		RegisterModel(BlockId::Tuff, "tuff.json");
		RegisterModel(BlockId::TuffBricks, "tuff_bricks.json");
		RegisterModel(BlockId::TwistingVines, "twisting_vines.json");
		RegisterModel(BlockId::TwistingVinesPlant, "twisting_vines_plant.json");
		RegisterModel(BlockId::VerdantFroglight, "verdant_froglight.json");
		RegisterModel(BlockId::VoidAir, "void_air.json");
		RegisterModel(BlockId::WarpedFungus, "warped_fungus.json");
		RegisterModel(BlockId::WarpedHyphae, "warped_hyphae.json");
		RegisterModel(BlockId::WarpedNylium, "warped_nylium.json");
		RegisterModel(BlockId::WarpedPlanks, "warped_planks.json");
		RegisterModel(BlockId::WarpedRoots, "warped_roots.json");
		RegisterModel(BlockId::WarpedStem, "warped_stem.json");
		RegisterModel(BlockId::WarpedWartBlock, "warped_wart_block.json");
		RegisterModel(BlockId::Water, "water.json");
		RegisterModel(BlockId::WaxedChiseledCopper, "waxed_chiseled_copper.json");
		RegisterModel(BlockId::WaxedCopperBlock, "waxed_copper_block.json");
		RegisterModel(BlockId::WaxedCopperChain, "waxed_copper_chain.json");
		RegisterModel(BlockId::WaxedCopperGrate, "waxed_copper_grate.json");
		RegisterModel(BlockId::WaxedCutCopper, "waxed_cut_copper.json");
		RegisterModel(BlockId::WaxedExposedChiseledCopper, "waxed_exposed_chiseled_copper.json");
		RegisterModel(BlockId::WaxedExposedCopper, "waxed_exposed_copper.json");
		RegisterModel(BlockId::WaxedExposedCopperChain, "waxed_exposed_copper_chain.json");
		RegisterModel(BlockId::WaxedExposedCopperGrate, "waxed_exposed_copper_grate.json");
		RegisterModel(BlockId::WaxedExposedCutCopper, "waxed_exposed_cut_copper.json");
		RegisterModel(BlockId::WaxedOxidizedChiseledCopper, "waxed_oxidized_chiseled_copper.json");
		RegisterModel(BlockId::WaxedOxidizedCopper, "waxed_oxidized_copper.json");
		RegisterModel(BlockId::WaxedOxidizedCopperChain, "waxed_oxidized_copper_chain.json");
		RegisterModel(BlockId::WaxedOxidizedCopperGrate, "waxed_oxidized_copper_grate.json");
		RegisterModel(BlockId::WaxedOxidizedCutCopper, "waxed_oxidized_cut_copper.json");
		RegisterModel(BlockId::WaxedWeatheredChiseledCopper, "waxed_weathered_chiseled_copper.json");
		RegisterModel(BlockId::WaxedWeatheredCopper, "waxed_weathered_copper.json");
		RegisterModel(BlockId::WaxedWeatheredCopperChain, "waxed_weathered_copper_chain.json");
		RegisterModel(BlockId::WaxedWeatheredCopperGrate, "waxed_weathered_copper_grate.json");
		RegisterModel(BlockId::WaxedWeatheredCutCopper, "waxed_weathered_cut_copper.json");
		RegisterModel(BlockId::WeatheredChiseledCopper, "weathered_chiseled_copper.json");
		RegisterModel(BlockId::WeatheredCopper, "weathered_copper.json");
		RegisterModel(BlockId::WeatheredCopperChain, "weathered_copper_chain.json");
		RegisterModel(BlockId::WeatheredCopperGrate, "weathered_copper_grate.json");
		RegisterModel(BlockId::WeatheredCutCopper, "weathered_cut_copper.json");
		RegisterModel(BlockId::WeepingVines, "weeping_vines.json");
		RegisterModel(BlockId::WeepingVinesPlant, "weeping_vines_plant.json");
		RegisterModel(BlockId::WetSponge, "wet_sponge.json");
		RegisterModel(BlockId::WhiteCarpet, "white_carpet.json");
		RegisterModel(BlockId::WhiteConcrete, "white_concrete.json");
		RegisterModel(BlockId::WhiteConcretePowder, "white_concrete_powder.json");
		RegisterModel(BlockId::WhiteStainedGlass, "white_stained_glass.json");
		RegisterModel(BlockId::WhiteTerracotta, "white_terracotta.json");
		RegisterModel(BlockId::WhiteTulip, "white_tulip.json");
		RegisterModel(BlockId::WhiteWool, "white_wool.json");
		RegisterModel(BlockId::WitherRose, "wither_rose.json");
		RegisterModel(BlockId::WitherSkeletonSkull, "wither_skeleton_skull.json");
		RegisterModel(BlockId::WitherSkeletonWallSkull, "wither_skeleton_wall_skull.json");
		RegisterModel(BlockId::YellowCarpet, "yellow_carpet.json");
		RegisterModel(BlockId::YellowConcrete, "yellow_concrete.json");
		RegisterModel(BlockId::YellowConcretePowder, "yellow_concrete_powder.json");
		RegisterModel(BlockId::YellowStainedGlass, "yellow_stained_glass.json");
		RegisterModel(BlockId::YellowTerracotta, "yellow_terracotta.json");
		RegisterModel(BlockId::YellowWool, "yellow_wool.json");

		// ---- Custom Blocks (not in Minecraft, or with special texture requirements) ----
		RegisterModel(BlockId::SnowGrassBlock,
					  {TextureInfo{"block/snow.png", Tint::None},
					   TextureInfo{"block/dirt.png", Tint::None},
					   TextureInfo{"block/grass_block_snow.png", Tint::None},
					   TextureInfo{"block/grass_block_snow.png", Tint::None},
					   TextureInfo{"block/grass_block_snow.png", Tint::None},
					   TextureInfo{"block/grass_block_snow.png", Tint::None}},
					  Model::Block);

		// ---- Reload Atlas Textures ----
		m_Atlas->ReloadTextures(m_AllTextureNames);
	}

} // namespace onion::voxel
