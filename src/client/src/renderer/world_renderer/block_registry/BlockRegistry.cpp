#include "BlockRegistry.hpp"

#include "BlockModel.hpp"

namespace
{
	static std::string ToFilename(const std::string& texturePath)
	{
		// Remove folders
		size_t slashPos = texturePath.find_last_of('/');
		std::string name = (slashPos != std::string::npos) ? texturePath.substr(slashPos + 1) : texturePath;

		// Add extension
		return name + ".png";
	}

	std::string ResolveTexture(const std::string& ref, const onion::voxel::BlockModel::Textures& t)
	{
		if (!ref.starts_with('#'))
			return ToFilename(ref);

		std::string key = ref.substr(1);

		if (key == "top")
			return ResolveTexture(t.Top, t);
		if (key == "bottom")
			return ResolveTexture(t.Bottom, t);
		if (key == "side")
			return ResolveTexture(t.Side, t);
		if (key == "overlay")
			return ResolveTexture(t.Overlay, t);
		if (key == "all")
			return ResolveTexture(t.All, t);
		if (key == "down")
			return ResolveTexture(t.Down, t);
		if (key == "up")
			return ResolveTexture(t.Up, t);
		if (key == "north")
			return ResolveTexture(t.North, t);
		if (key == "south")
			return ResolveTexture(t.South, t);
		if (key == "west")
			return ResolveTexture(t.West, t);
		if (key == "east")
			return ResolveTexture(t.East, t);
		if (key == "end")
			return ResolveTexture(t.End, t);
		if (key == "front")
			return ResolveTexture(t.Front, t);
		if (key == "back")
			return ResolveTexture(t.Back, t);

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
} // namespace

namespace onion::voxel
{
	BlockRegistry::BlockRegistry(std::shared_ptr<TextureAtlas> atlas) : m_Atlas(atlas)
	{

		// Top
		// Bottom
		// Front
		// Back
		// Left
		// Right

		const std::filesystem::path blockModelDir = AssetsManager::GetAssetsDirectory() / "models" / "block";

		PreRegisterModel(BlockId::Stone, blockModelDir / "stone.json");
		PreRegisterModel(BlockId::Dirt, blockModelDir / "dirt.json");
		PreRegisterModel(BlockId::Grass, blockModelDir / "grass_block.json");
		PreRegisterModel(BlockId::Glass, blockModelDir / "glass.json");
		PreRegisterModel(BlockId::BlackStainedGlass, blockModelDir / "black_stained_glass.json");
		PreRegisterModel(BlockId::BlueStainedGlass, blockModelDir / "blue_stained_glass.json");
		PreRegisterModel(BlockId::BrownStainedGlass, blockModelDir / "brown_stained_glass.json");
		PreRegisterModel(BlockId::CyanStainedGlass, blockModelDir / "cyan_stained_glass.json");
		PreRegisterModel(BlockId::GrayStainedGlass, blockModelDir / "gray_stained_glass.json");
		PreRegisterModel(BlockId::GreenStainedGlass, blockModelDir / "green_stained_glass.json");
		PreRegisterModel(BlockId::LightBlueStainedGlass, blockModelDir / "light_blue_stained_glass.json");
		PreRegisterModel(BlockId::LightGrayStainedGlass, blockModelDir / "light_gray_stained_glass.json");
		PreRegisterModel(BlockId::LimeStainedGlass, blockModelDir / "lime_stained_glass.json");
		PreRegisterModel(BlockId::MagentaStainedGlass, blockModelDir / "magenta_stained_glass.json");
		PreRegisterModel(BlockId::OrangeStainedGlass, blockModelDir / "orange_stained_glass.json");
		PreRegisterModel(BlockId::PinkStainedGlass, blockModelDir / "pink_stained_glass.json");
		PreRegisterModel(BlockId::PurpleStainedGlass, blockModelDir / "purple_stained_glass.json");
		PreRegisterModel(BlockId::RedStainedGlass, blockModelDir / "red_stained_glass.json");
		PreRegisterModel(BlockId::WhiteStainedGlass, blockModelDir / "white_stained_glass.json");
		PreRegisterModel(BlockId::YellowStainedGlass, blockModelDir / "yellow_stained_glass.json");
		PreRegisterModel(BlockId::OakLog, blockModelDir / "oak_log.json");
		PreRegisterModel(BlockId::BirchLog, blockModelDir / "birch_log.json");
		PreRegisterModel(BlockId::SpruceLog, blockModelDir / "spruce_log.json");
		PreRegisterModel(BlockId::Bedrock, blockModelDir / "bedrock.json");
		PreRegisterModel(BlockId::Sand, blockModelDir / "sand.json");
		PreRegisterModel(BlockId::Gravel, blockModelDir / "gravel.json");
		PreRegisterModel(BlockId::Cobblestone, blockModelDir / "cobblestone.json");
		PreRegisterModel(BlockId::SnowBlock, blockModelDir / "snow_block.json");
		PreRegisterModel(BlockId::OakLeaves, blockModelDir / "oak_leaves.json");
		PreRegisterModel(BlockId::BirchLeaves, blockModelDir / "birch_leaves.json");
		PreRegisterModel(BlockId::SpruceLeaves, blockModelDir / "spruce_leaves.json");
		PreRegisterModel(BlockId::Ice, blockModelDir / "ice.json");
		PreRegisterModel(BlockId::Furnace, blockModelDir / "furnace.json");
		PreRegisterModel(BlockId::Water, blockModelDir / "water.json");
		PreRegisterModel(BlockId::Sandstone, blockModelDir / "sandstone.json");

		PreRegister(BlockId::Poppy, TextureInfo{"poppy.png", Tint::None, Transparency::Cutout}, Model::Cross);
		PreRegister(BlockId::Dandelion, TextureInfo{"dandelion.png", Tint::None, Transparency::Cutout}, Model::Cross);

		PreRegister(
			BlockId::BrownMushroom, TextureInfo{"brown_mushroom.png", Tint::None, Transparency::Cutout}, Model::Cross);
		PreRegister(
			BlockId::RedMushroom, TextureInfo{"red_mushroom.png", Tint::None, Transparency::Cutout}, Model::Cross);
		PreRegister(BlockId::Cobweb, TextureInfo{"cobweb.png", Tint::None, Transparency::Cutout}, Model::Cross);
		PreRegister(BlockId::Kelp, TextureInfo{"kelp.png", Tint::Foliage, Transparency::Cutout}, Model::Cross);
		PreRegister(BlockId::DeadBush, TextureInfo{"dead_bush.png", Tint::None, Transparency::Cutout}, Model::Cross);
		PreRegister(
			BlockId::OakSapling, TextureInfo{"oak_sapling.png", Tint::None, Transparency::Cutout}, Model::Cross);
		PreRegister(BlockId::RedTulip, TextureInfo{"red_tulip.png", Tint::None, Transparency::Cutout}, Model::Cross);
		PreRegister(
			BlockId::OrangeTulip, TextureInfo{"orange_tulip.png", Tint::None, Transparency::Cutout}, Model::Cross);
		PreRegister(
			BlockId::WhiteTulip, TextureInfo{"white_tulip.png", Tint::None, Transparency::Cutout}, Model::Cross);
		PreRegister(BlockId::PinkTulip, TextureInfo{"pink_tulip.png", Tint::None, Transparency::Cutout}, Model::Cross);
		PreRegister(
			BlockId::ShortGrass, TextureInfo{"short_grass.png", Tint::Foliage, Transparency::Cutout}, Model::Cross);

		PreRegister(BlockId::SnowGrass,
					{TextureInfo{"snow.png", Tint::None, Transparency::Opaque},
					 TextureInfo{"dirt.png", Tint::None, Transparency::Opaque},
					 TextureInfo{"grass_block_snow.png", Tint::None, Transparency::Opaque},
					 TextureInfo{"grass_block_snow.png", Tint::None, Transparency::Opaque},
					 TextureInfo{"grass_block_snow.png", Tint::None, Transparency::Opaque},
					 TextureInfo{"grass_block_snow.png", Tint::None, Transparency::Opaque}});

		PreRegister(BlockId::Cactus,
					{TextureInfo{"cactus_top.png", Tint::None, Transparency::Cutout},
					 TextureInfo{"cactus_bottom.png", Tint::None, Transparency::Cutout},
					 TextureInfo{"cactus_side.png", Tint::None, Transparency::Cutout},
					 TextureInfo{"cactus_side.png", Tint::None, Transparency::Cutout},
					 TextureInfo{"cactus_side.png", Tint::None, Transparency::Cutout},
					 TextureInfo{"cactus_side.png", Tint::None, Transparency::Cutout}});

		PreRegister(
			BlockId::CactusFlower, TextureInfo{"cactus_flower.png", Tint::None, Transparency::Cutout}, Model::Cross);
	}

	void BlockRegistry::PreRegister(BlockId id, const std::array<TextureInfo, 6>& textures, Model textureModel)
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

	void BlockRegistry::PreRegister(BlockId id, const TextureInfo& texture, Model textureModel)
	{
		std::array<TextureInfo, 6> textures;
		textures.fill(texture);
		PreRegister(id, textures, textureModel);
	}

	void BlockRegistry::PreRegister(BlockId id, const std::string& texture, Model textureModel)
	{
		std::array<TextureInfo, 6> textures;
		textures.fill({texture});
		PreRegister(id, textures, textureModel);
	}

	void BlockRegistry::PreSetOverlay(BlockId id, Face face, const TextureInfo& texture)
	{
		m_AllTextureNames.insert(texture.name);
		m_RegistrationsOverlays.emplace_back(id, face, texture);
	}

	void BlockRegistry::PreRegisterModel(BlockId id, const std::filesystem::path& model)
	{
		BlockModel blockModel = BlockModel::FromFile(model);

		// Magic Tricks
		if (id == BlockId::Water)
		{
			// Load stone.json as base model to get the geometry, but use water texture
			BlockModel stoneModel =
				BlockModel::FromFile(AssetsManager::GetAssetsDirectory() / "models" / "block" / "stone.json");
			stoneModel.ModelTextures.All = blockModel.ModelTextures.Particle; // Use water texture
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

		for (size_t elemIndex = 0; elemIndex < blockModel.Elements.size(); elemIndex++)
		{
			const auto& elem = blockModel.Elements[elemIndex];
			bool isOverlay = (elemIndex > 0);

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
					bool transparent = BlockState::IsTransparent(id);
					Transparency texTransparency = transparent ? Transparency::Transparent : Transparency::Opaque;
					baseTextures[(int) f] = TextureInfo{resolved, tint, texTransparency};
					baseInitialized = true;
				}
				else
				{
					PreSetOverlay(id, f, TextureInfo{resolved, tint, Transparency::Cutout});
				}
			}
		}

		if (baseInitialized)
		{
			PreRegister(id, baseTextures);
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
			tex.faces[i].texture = m_Atlas->GetTextureID(textures[i].name);
			tex.faces[i].tintType = textures[i].tintType;
			tex.faces[i].textureType = textures[i].textureType;

			m_AllTextureNames.insert(textures[i].name);
		}

		m_Blocks[id] = tex;
	}

	void BlockRegistry::SetOverlay(BlockId id, Face face, const TextureInfo& texture)
	{
		auto it = m_Blocks.find(id);
		if (it == m_Blocks.end())
		{
			throw std::runtime_error("BlockRegistry::SetOverlay: Block ID not found: " +
									 std::to_string(static_cast<uint8_t>(id)));
		}

		it->second.overlay[static_cast<size_t>(face)].texture = m_Atlas->GetTextureID(texture.name);
		it->second.overlay[static_cast<size_t>(face)].tintType = texture.tintType;
		it->second.overlay[static_cast<size_t>(face)].textureType = texture.textureType;

		m_AllTextureNames.insert(texture.name);
	}

	void BlockRegistry::Initialize()
	{
		ReloadTextures();
	}

	void BlockRegistry::ReloadTextures()
	{
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
									 std::to_string(static_cast<uint8_t>(id)));
		}

		return it->second;
	}
} // namespace onion::voxel
