#include "BlockRegistry.hpp"

#include "BlockModel.hpp"

namespace
{
	static std::string ToFilename(const std::string& texturePath)
	{
		// Add extension
		return texturePath + ".png";
	}

	std::string ResolveTexture(const std::string& ref, const onion::voxel::BlockModel::Textures& t)
	{
		if (!ref.starts_with('#'))
			return ToFilename(ref);

		const std::string key = ref.substr(1);
		auto it = t.find(key);
		if (it != t.end())
			return ResolveTexture(it->second, t);

		return "";
	}

	onion::voxel::Face ToFace(const std::string& name)
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

	bool IsOverlay(const onion::voxel::BlockModel::Element& elem)
	{
		// Overlay if all textures points to "#overlay"
		for (const auto& [faceName, face] : elem.Faces)
		{
			if (face.Texture != "#overlay")
				return false;
		}
		return true;
	}
} // namespace

namespace onion::voxel
{
	BlockRegistry::BlockRegistry(std::shared_ptr<TextureAtlas> atlas) : m_Atlas(atlas) {}

	void BlockRegistry::RegisterModel(BlockId id, const std::array<TextureInfo, 6>& textures, Model textureModel)
	{
		for (const auto& texture : textures)
		{
			m_AllTextureNames.insert(texture.name);
		}

		PreRegistration registration;
		registration.id = id;
		registration.textures = textures;
		registration.textureModel = textureModel;

		m_Registrations.emplace_back(registration);
	}

	void BlockRegistry::PreSetOverlay(BlockId id, Face face, const TextureInfo& texture)
	{
		m_AllTextureNames.insert(texture.name);
		m_RegistrationsOverlays.emplace_back(id, face, texture);
	}

	void BlockRegistry::RegisterModel(BlockId id, const std::filesystem::path& model)
	{
		BlockModel blockModel = BlockModel::FromFile(model);

		// ----- Magic trick for water block ----
		if (id == BlockId::Water)
		{
			// Load stone.json as base model to get the geometry, but use water texture
			BlockModel stoneModel = BlockModel::FromFile("stone.json");
			stoneModel.ModelTextures["all"] = blockModel.ModelTextures["particle"]; // Use water texture
			blockModel = stoneModel;

			// Set Tint to Water for all faces
			for (auto& elem : blockModel.Elements)
			{
				for (auto& [faceName, face] : elem.Faces)
				{
					face.TintIndex = 1; // Corresponds to Tint::Water
				}
			}
		}

		std::array<TextureInfo, 6> baseTextures{};
		bool baseInitialized = false;

		Model textureModel = Model::Block;

		// ----- Magic trick for blocks with cross textures -----
		if (!blockModel.ModelTextures["cross"].empty())
		{
			textureModel = Model::Cross;
			// Load stone.json as base model to get the geometry, but use cross texture
			BlockModel stoneModel = BlockModel::FromFile("stone.json");
			stoneModel.ModelTextures["all"] = blockModel.ModelTextures["cross"]; // Use cross texture
			blockModel = stoneModel;

			// Set Tint to ShortGrass for all faces
			if (id == BlockId::ShortGrass)
			{
				for (auto& elem : blockModel.Elements)
				{
					for (auto& [faceName, face] : elem.Faces)
					{
						face.TintIndex = 0; // Corresponds to Tint::Grass
					}
				}
			}
		}

		for (size_t elemIndex = 0; elemIndex < blockModel.Elements.size(); elemIndex++)
		{
			const auto& elem = blockModel.Elements[elemIndex];

			bool isOverlay = IsOverlay(elem);

			for (const auto& [faceName, face] : elem.Faces)
			{
				Face f = ToFace(faceName);

				std::string resolved = ResolveTexture(face.Texture, blockModel.ModelTextures);

				if (resolved.empty())
				{
					throw std::runtime_error("Failed to resolve texture reference: " + face.Texture);
				}

				Tint tint = Tint::None;
				if (face.TintIndex.has_value() && face.TintIndex.value() == 0)
					tint = Tint::Grass;
				else if (face.TintIndex.has_value() && face.TintIndex.value() == 1)
					tint = Tint::Water;

				if (!isOverlay)
				{
					glm::u8vec3 from = {elem.From[0], elem.From[1], elem.From[2]};
					glm::u8vec3 to = {elem.To[0], elem.To[1], elem.To[2]};
					baseTextures[(int) f] = TextureInfo{resolved, tint, from, to};
					baseInitialized = true;
				}
				else
				{
					PreSetOverlay(id, f, TextureInfo{resolved, tint});
				}
			}
		}

		if (baseInitialized)
		{
			RegisterModel(id, baseTextures, textureModel);
		}

		return;
	}

	void BlockRegistry::Register(BlockId id, const std::array<TextureInfo, 6>& textures, Model textureModel)
	{
		BlockTextures tex;
		tex.rotationType = BlockState::GetRotationType(id);
		tex.textureModel = textureModel;

		for (size_t i = 0; i < 6; i++)
		{
			const std::string& textureName = textures[i].name;
			if (textureName.empty())
			{
				return;
			}

			tex.faces[i].texture = m_Atlas->GetTextureID(textureName);
			tex.faces[i].tintType = textures[i].tintType;
			tex.faces[i].textureType = m_Atlas->GetTextureTransparency(textureName);
			tex.faces[i].from = textures[i].from;
			tex.faces[i].to = textures[i].to;

			m_AllTextureNames.insert(textureName);
		}

		m_Blocks[id] = tex;
	}

	void BlockRegistry::SetOverlay(BlockId id, Face face, const TextureInfo& texture)
	{
		auto it = m_Blocks.find(id);
		if (it == m_Blocks.end())
		{
			throw std::runtime_error("BlockRegistry::SetOverlay: Block ID not found: " +
									 std::to_string(static_cast<uint16_t>(id)));
		}

		it->second.overlay[static_cast<size_t>(face)].texture = m_Atlas->GetTextureID(texture.name);
		it->second.overlay[static_cast<size_t>(face)].tintType = texture.tintType;
		it->second.overlay[static_cast<size_t>(face)].textureType = m_Atlas->GetTextureTransparency(texture.name);

		m_AllTextureNames.insert(texture.name);
	}

	void BlockRegistry::Initialize()
	{
		ReloadTextures();
	}

	void BlockRegistry::ReloadTextures()
	{
		// Reload Models
		ReloadModels();

		// Clear existing data
		m_Blocks.clear();

		// Process pending registrations
		for (const auto& registration : m_Registrations)
		{
			Register(registration.id, registration.textures, registration.textureModel);
		}

		// Process pending overlays
		for (const auto& overlay : m_RegistrationsOverlays)
		{
			SetOverlay(overlay.id, overlay.face, overlay.texture);
		}
	}

	const std::unordered_set<std::string>& BlockRegistry::GetAllTextureNames() const
	{
		return m_AllTextureNames;
	}

	const BlockTextures& BlockRegistry::Get(BlockId id) const
	{
		auto it = m_Blocks.find(id);
		if (it == m_Blocks.end())
		{
			throw std::runtime_error("BlockRegistry::Get: Block ID not found: " +
									 std::to_string(static_cast<uint16_t>(id)));
		}

		return it->second;
	}

	void BlockRegistry::ReloadModels()
	{
		BlockModel::ClearCache();

		m_AllTextureNames.clear();
		m_Registrations.clear();
		m_RegistrationsOverlays.clear();

		RegisterModel(BlockId::Stone, "stone.json");
		RegisterModel(BlockId::Dirt, "dirt.json");
		RegisterModel(BlockId::Grass, "grass_block.json");
		RegisterModel(BlockId::Glass, "glass.json");
		RegisterModel(BlockId::BlackStainedGlass, "black_stained_glass.json");
		RegisterModel(BlockId::BlueStainedGlass, "blue_stained_glass.json");
		RegisterModel(BlockId::BrownStainedGlass, "brown_stained_glass.json");
		RegisterModel(BlockId::CyanStainedGlass, "cyan_stained_glass.json");
		RegisterModel(BlockId::GrayStainedGlass, "gray_stained_glass.json");
		RegisterModel(BlockId::GreenStainedGlass, "green_stained_glass.json");
		RegisterModel(BlockId::LightBlueStainedGlass, "light_blue_stained_glass.json");
		RegisterModel(BlockId::LightGrayStainedGlass, "light_gray_stained_glass.json");
		RegisterModel(BlockId::LimeStainedGlass, "lime_stained_glass.json");
		RegisterModel(BlockId::MagentaStainedGlass, "magenta_stained_glass.json");
		RegisterModel(BlockId::OrangeStainedGlass, "orange_stained_glass.json");
		RegisterModel(BlockId::PinkStainedGlass, "pink_stained_glass.json");
		RegisterModel(BlockId::PurpleStainedGlass, "purple_stained_glass.json");
		RegisterModel(BlockId::RedStainedGlass, "red_stained_glass.json");
		RegisterModel(BlockId::WhiteStainedGlass, "white_stained_glass.json");
		RegisterModel(BlockId::YellowStainedGlass, "yellow_stained_glass.json");
		RegisterModel(BlockId::OakLog, "oak_log.json");
		RegisterModel(BlockId::BirchLog, "birch_log.json");
		RegisterModel(BlockId::SpruceLog, "spruce_log.json");
		RegisterModel(BlockId::Bedrock, "bedrock.json");
		RegisterModel(BlockId::Sand, "sand.json");
		RegisterModel(BlockId::Gravel, "gravel.json");
		RegisterModel(BlockId::Cobblestone, "cobblestone.json");
		RegisterModel(BlockId::SnowBlock, "snow_block.json");
		RegisterModel(BlockId::OakLeaves, "oak_leaves.json");
		RegisterModel(BlockId::BirchLeaves, "birch_leaves.json");
		RegisterModel(BlockId::SpruceLeaves, "spruce_leaves.json");
		RegisterModel(BlockId::Ice, "ice.json");
		RegisterModel(BlockId::Furnace, "furnace.json");
		RegisterModel(BlockId::Water, "water.json");
		RegisterModel(BlockId::Sandstone, "sandstone.json");
		RegisterModel(BlockId::Poppy, "poppy.json");
		RegisterModel(BlockId::Dandelion, "dandelion.json");
		RegisterModel(BlockId::BrownMushroom, "brown_mushroom.json");
		RegisterModel(BlockId::RedMushroom, "red_mushroom.json");
		RegisterModel(BlockId::Cobweb, "cobweb.json");
		RegisterModel(BlockId::Kelp, "kelp.json");
		RegisterModel(BlockId::DeadBush, "dead_bush.json");
		RegisterModel(BlockId::OakSapling, "oak_sapling.json");
		RegisterModel(BlockId::RedTulip, "red_tulip.json");
		RegisterModel(BlockId::OrangeTulip, "orange_tulip.json");
		RegisterModel(BlockId::WhiteTulip, "white_tulip.json");
		RegisterModel(BlockId::PinkTulip, "pink_tulip.json");
		RegisterModel(BlockId::ShortGrass, "short_grass.json");
		RegisterModel(BlockId::CactusFlower, "cactus_flower.json");
		RegisterModel(BlockId::Cactus, "cactus.json");

		RegisterModel(BlockId::AcaciaLeaves, "acacia_leaves.json");
		RegisterModel(BlockId::AcaciaLog, "acacia_log.json");
		RegisterModel(BlockId::AcaciaLogHorizontal, "acacia_log_horizontal.json");
		RegisterModel(BlockId::AcaciaPlanks, "acacia_planks.json");
		RegisterModel(BlockId::AcaciaSapling, "acacia_sapling.json");
		RegisterModel(BlockId::AcaciaWood, "acacia_wood.json");
		RegisterModel(BlockId::Allium, "allium.json");
		RegisterModel(BlockId::AmethystBlock, "amethyst_block.json");
		RegisterModel(BlockId::AmethystCluster, "amethyst_cluster.json");
		RegisterModel(BlockId::AncientDebris, "ancient_debris.json");

		// ---- Custom Blocks that do not exist in Minecraft, or have special texture requirements ----
		RegisterModel(BlockId::SnowGrass,
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
