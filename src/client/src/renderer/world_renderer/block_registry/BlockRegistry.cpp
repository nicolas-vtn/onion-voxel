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

		Register(BlockId::Stone, "stone.png");

		Register(BlockId::Dirt, "dirt.png");

		std::array<TextureInfo, 6> grassTextures = {
			TextureInfo{"grass_block_top.png", TintType::Grass, TextureType::Opaque},
			TextureInfo{"dirt.png", TintType::None, TextureType::Opaque},
			TextureInfo{"grass_block_side.png", TintType::None, TextureType::Opaque},
			TextureInfo{"grass_block_side.png", TintType::None, TextureType::Opaque},
			TextureInfo{"grass_block_side.png", TintType::None, TextureType::Opaque},
			TextureInfo{"grass_block_side.png", TintType::None, TextureType::Opaque}};
		Register(BlockId::Grass, grassTextures);
		SetOverlay(BlockId::Grass,
				   BlockFace::Front,
				   TextureInfo{"grass_block_side_overlay.png", TintType::Grass, TextureType::Cutout});
		SetOverlay(BlockId::Grass,
				   BlockFace::Back,
				   TextureInfo{"grass_block_side_overlay.png", TintType::Grass, TextureType::Cutout});
		SetOverlay(BlockId::Grass,
				   BlockFace::Left,
				   TextureInfo{"grass_block_side_overlay.png", TintType::Grass, TextureType::Cutout});
		SetOverlay(BlockId::Grass,
				   BlockFace::Right,
				   TextureInfo{"grass_block_side_overlay.png", TintType::Grass, TextureType::Cutout});

		Register(BlockId::Glass, TextureInfo{"light_blue_stained_glass.png", TintType::None, TextureType::Transparent});

		Register(BlockId::OakLog,
				 {TextureInfo{"oak_log.png", TintType::None, TextureType::Opaque},
				  TextureInfo{"oak_log.png", TintType::None, TextureType::Opaque},
				  TextureInfo{"oak_log_top.png", TintType::None, TextureType::Opaque},
				  TextureInfo{"oak_log_top.png", TintType::None, TextureType::Opaque},
				  TextureInfo{"oak_log.png", TintType::None, TextureType::Opaque},
				  TextureInfo{"oak_log.png", TintType::None, TextureType::Opaque}});

		Register(BlockId::OakLeaves, TextureInfo{"oak_leaves.png", TintType::Foliage, TextureType::Cutout});

		Register(BlockId::Furnace,
				 {TextureInfo{"furnace_top.png", TintType::None, TextureType::Opaque},
				  TextureInfo{"furnace_top.png", TintType::None, TextureType::Opaque},
				  TextureInfo{"furnace_front.png", TintType::None, TextureType::Opaque},
				  TextureInfo{"furnace_side.png", TintType::None, TextureType::Opaque},
				  TextureInfo{"furnace_side.png", TintType::None, TextureType::Opaque},
				  TextureInfo{"furnace_side.png", TintType::None, TextureType::Opaque}});

		Register(BlockId::Bedrock, "bedrock.png");

		Register(BlockId::Water, TextureInfo{"water_still.png", TintType::Water, TextureType::Transparent});

		Register(BlockId::Sand, "sand.png");

		Register(BlockId::Gravel, "gravel.png");

		Register(BlockId::Cobblestone, "cobblestone.png");
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
		}

		m_Blocks[id] = tex;
	}

	void BlockRegistry::Register(BlockId id, const TextureInfo& texture)
	{
		std::array<TextureInfo, 6> textures;
		textures.fill(texture);
		Register(id, textures);
	}

	void BlockRegistry::Register(BlockId id, const std::string& texture)
	{
		std::array<TextureInfo, 6> textures;
		textures.fill({texture});
		Register(id, textures);
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
