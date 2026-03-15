#include "BlockRegistry.hpp"

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

		PreRegister(BlockId::Stone, "stone.png");

		PreRegister(BlockId::Dirt, "dirt.png");

		std::array<TextureInfo, 6> grassTextures = {
			TextureInfo{"grass_block_top.png", Tint::Grass, Transparency::Opaque},
			TextureInfo{"dirt.png", Tint::None, Transparency::Opaque},
			TextureInfo{"grass_block_side.png", Tint::None, Transparency::Opaque},
			TextureInfo{"grass_block_side.png", Tint::None, Transparency::Opaque},
			TextureInfo{"grass_block_side.png", Tint::None, Transparency::Opaque},
			TextureInfo{"grass_block_side.png", Tint::None, Transparency::Opaque}};
		PreRegister(BlockId::Grass, grassTextures);
		PreSetOverlay(BlockId::Grass,
					  Face::Front,
					  TextureInfo{"grass_block_side_overlay.png", Tint::Grass, Transparency::Cutout});
		PreSetOverlay(
			BlockId::Grass, Face::Back, TextureInfo{"grass_block_side_overlay.png", Tint::Grass, Transparency::Cutout});
		PreSetOverlay(
			BlockId::Grass, Face::Left, TextureInfo{"grass_block_side_overlay.png", Tint::Grass, Transparency::Cutout});
		PreSetOverlay(BlockId::Grass,
					  Face::Right,
					  TextureInfo{"grass_block_side_overlay.png", Tint::Grass, Transparency::Cutout});

		PreRegister(BlockId::Glass, TextureInfo{"light_blue_stained_glass.png", Tint::None, Transparency::Transparent});
		PreRegister(BlockId::OakLog,
					{TextureInfo{"oak_log.png", Tint::None, Transparency::Opaque},
					 TextureInfo{"oak_log.png", Tint::None, Transparency::Opaque},
					 TextureInfo{"oak_log_top.png", Tint::None, Transparency::Opaque},
					 TextureInfo{"oak_log_top.png", Tint::None, Transparency::Opaque},
					 TextureInfo{"oak_log.png", Tint::None, Transparency::Opaque},
					 TextureInfo{"oak_log.png", Tint::None, Transparency::Opaque}});

		PreRegister(BlockId::OakLeaves, TextureInfo{"oak_leaves.png", Tint::Foliage, Transparency::Cutout});

		PreRegister(BlockId::Furnace,
					{TextureInfo{"furnace_top.png", Tint::None, Transparency::Opaque},
					 TextureInfo{"furnace_top.png", Tint::None, Transparency::Opaque},
					 TextureInfo{"furnace_front.png", Tint::None, Transparency::Opaque},
					 TextureInfo{"furnace_side.png", Tint::None, Transparency::Opaque},
					 TextureInfo{"furnace_side.png", Tint::None, Transparency::Opaque},
					 TextureInfo{"furnace_side.png", Tint::None, Transparency::Opaque}});

		PreRegister(BlockId::Bedrock, "bedrock.png");

		PreRegister(BlockId::Water, TextureInfo{"water_still.png", Tint::Water, Transparency::Transparent});

		PreRegister(BlockId::Sand, "sand.png");
		PreRegister(BlockId::Sandstone,
					{TextureInfo{"sandstone_top.png", Tint::None, Transparency::Opaque},
					 TextureInfo{"sandstone_bottom.png", Tint::None, Transparency::Opaque},
					 TextureInfo{"sandstone.png", Tint::None, Transparency::Opaque},
					 TextureInfo{"sandstone.png", Tint::None, Transparency::Opaque},
					 TextureInfo{"sandstone.png", Tint::None, Transparency::Opaque},
					 TextureInfo{"sandstone.png", Tint::None, Transparency::Opaque}});

		PreRegister(BlockId::Gravel, "gravel.png");

		PreRegister(BlockId::Cobblestone, "cobblestone.png");

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

		PreRegister(BlockId::SnowBlock, "snow.png");

		PreRegister(BlockId::BirchLog,
					{TextureInfo{"birch_log.png", Tint::None, Transparency::Opaque},
					 TextureInfo{"birch_log.png", Tint::None, Transparency::Opaque},
					 TextureInfo{"birch_log_top.png", Tint::None, Transparency::Opaque},
					 TextureInfo{"birch_log_top.png", Tint::None, Transparency::Opaque},
					 TextureInfo{"birch_log.png", Tint::None, Transparency::Opaque},
					 TextureInfo{"birch_log.png", Tint::None, Transparency::Opaque}});

		PreRegister(BlockId::BirchLeaves, TextureInfo{"birch_leaves.png", Tint::Foliage, Transparency::Cutout});

		PreRegister(BlockId::Ice, TextureInfo{"ice.png", Tint::None, Transparency::Transparent});

		PreRegister(BlockId::SpruceLog,
					{TextureInfo{"spruce_log.png", Tint::None, Transparency::Opaque},
					 TextureInfo{"spruce_log.png", Tint::None, Transparency::Opaque},
					 TextureInfo{"spruce_log_top.png", Tint::None, Transparency::Opaque},
					 TextureInfo{"spruce_log_top.png", Tint::None, Transparency::Opaque},
					 TextureInfo{"spruce_log.png", Tint::None, Transparency::Opaque},
					 TextureInfo{"spruce_log.png", Tint::None, Transparency::Opaque}});

		PreRegister(BlockId::SpruceLeaves, TextureInfo{"spruce_leaves.png", Tint::Foliage, Transparency::Cutout});

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
