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
			TextureInfo{"grass_block_top.png", TintType::Grass, TextureType::Opaque},
			TextureInfo{"dirt.png", TintType::None, TextureType::Opaque},
			TextureInfo{"grass_block_side.png", TintType::None, TextureType::Opaque},
			TextureInfo{"grass_block_side.png", TintType::None, TextureType::Opaque},
			TextureInfo{"grass_block_side.png", TintType::None, TextureType::Opaque},
			TextureInfo{"grass_block_side.png", TintType::None, TextureType::Opaque}};
		PreRegister(BlockId::Grass, grassTextures);
		PreSetOverlay(BlockId::Grass,
					  BlockFace::Front,
					  TextureInfo{"grass_block_side_overlay.png", TintType::Grass, TextureType::Cutout});
		PreSetOverlay(BlockId::Grass,
					  BlockFace::Back,
					  TextureInfo{"grass_block_side_overlay.png", TintType::Grass, TextureType::Cutout});
		PreSetOverlay(BlockId::Grass,
					  BlockFace::Left,
					  TextureInfo{"grass_block_side_overlay.png", TintType::Grass, TextureType::Cutout});
		PreSetOverlay(BlockId::Grass,
					  BlockFace::Right,
					  TextureInfo{"grass_block_side_overlay.png", TintType::Grass, TextureType::Cutout});

		PreRegister(BlockId::Glass,
					TextureInfo{"light_blue_stained_glass.png", TintType::None, TextureType::Transparent});
		PreRegister(BlockId::OakLog,
					{TextureInfo{"oak_log.png", TintType::None, TextureType::Opaque},
					 TextureInfo{"oak_log.png", TintType::None, TextureType::Opaque},
					 TextureInfo{"oak_log_top.png", TintType::None, TextureType::Opaque},
					 TextureInfo{"oak_log_top.png", TintType::None, TextureType::Opaque},
					 TextureInfo{"oak_log.png", TintType::None, TextureType::Opaque},
					 TextureInfo{"oak_log.png", TintType::None, TextureType::Opaque}});

		PreRegister(BlockId::OakLeaves, TextureInfo{"oak_leaves.png", TintType::Foliage, TextureType::Cutout});

		PreRegister(BlockId::Furnace,
					{TextureInfo{"furnace_top.png", TintType::None, TextureType::Opaque},
					 TextureInfo{"furnace_top.png", TintType::None, TextureType::Opaque},
					 TextureInfo{"furnace_front.png", TintType::None, TextureType::Opaque},
					 TextureInfo{"furnace_side.png", TintType::None, TextureType::Opaque},
					 TextureInfo{"furnace_side.png", TintType::None, TextureType::Opaque},
					 TextureInfo{"furnace_side.png", TintType::None, TextureType::Opaque}});

		PreRegister(BlockId::Bedrock, "bedrock.png");

		PreRegister(BlockId::Water, TextureInfo{"water_still.png", TintType::Water, TextureType::Transparent});

		PreRegister(BlockId::Sand, "sand.png");

		PreRegister(BlockId::Gravel, "gravel.png");

		PreRegister(BlockId::Cobblestone, "cobblestone.png");
	}

	void BlockRegistry::PreRegister(BlockId id, const std::array<TextureInfo, 6>& textures)
	{
		for (const auto& texture : textures)
		{
			m_AllTextureNames.insert(texture.name);
		}

		m_Registrations.emplace_back(id, textures);
	}

	void BlockRegistry::PreRegister(BlockId id, const TextureInfo& texture)
	{
		std::array<TextureInfo, 6> textures;
		textures.fill(texture);
		PreRegister(id, textures);
	}

	void BlockRegistry::PreRegister(BlockId id, const std::string& texture)
	{
		std::array<TextureInfo, 6> textures;
		textures.fill({texture});
		PreRegister(id, textures);
	}

	void BlockRegistry::PreSetOverlay(BlockId id, BlockFace face, const TextureInfo& texture)
	{
		m_AllTextureNames.insert(texture.name);
		m_RegistrationsOverlays.emplace_back(id, face, texture);
	}

	void BlockRegistry::Register(BlockId id, const std::array<TextureInfo, 6>& textures)
	{
		BlockTextures tex;
		tex.rotationType = Block::GetRotationType(id);

		for (size_t i = 0; i < 6; i++)
		{
			tex.faces[i].texture = m_Atlas->GetTextureID(textures[i].name);
			tex.faces[i].tintType = textures[i].tintType;
			tex.faces[i].textureType = textures[i].textureType;

			m_AllTextureNames.insert(textures[i].name);
		}

		m_Blocks[id] = tex;
	}

	void BlockRegistry::SetOverlay(BlockId id, BlockFace face, const TextureInfo& texture)
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
		for (const auto& [id, textures] : m_Registrations)
		{
			Register(id, textures);
		}

		// Process pending overlays
		for (const auto& [id, face, texture] : m_RegistrationsOverlays)
		{
			SetOverlay(id, face, texture);
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
