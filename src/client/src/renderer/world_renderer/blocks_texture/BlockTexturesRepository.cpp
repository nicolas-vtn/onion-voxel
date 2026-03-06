#include "BlockTexturesRepository.hpp"

namespace onion::voxel
{
	// ----- Static Initialization -----
	const BlockTexturesRepository::BlockTexturesNames BlockTexturesRepository::s_Stone{"stone.png"};
	const BlockTexturesRepository::BlockTexturesNames BlockTexturesRepository::s_Dirt{"dirt.png"};
	const BlockTexturesRepository::BlockTexturesNames BlockTexturesRepository::s_Grass{"grass_block_top.png",
																					   true,
																					   "dirt.png",
																					   false,
																					   "grass_block_side.png",
																					   false,
																					   "grass_block_side.png",
																					   false,
																					   "grass_block_side.png",
																					   false,
																					   "grass_block_side.png",
																					   false,
																					   "",
																					   false,
																					   "",
																					   false,
																					   "grass_block_side_overlay.png",
																					   true,
																					   "grass_block_side_overlay.png",
																					   true,
																					   "grass_block_side_overlay.png",
																					   true,
																					   "grass_block_side_overlay.png",
																					   true};

	const std::unordered_map<BlockId, BlockTexturesRepository::BlockTexturesNames>
		BlockTexturesRepository::s_BlockTexturesNamesMap{
			{BlockId::Stone, s_Stone}, {BlockId::Dirt, s_Dirt}, {BlockId::Grass, s_Grass}};

	void BlockTexturesRepository::Initialize(const TextureAtlas& textureAtlas)
	{
		s_BlockTexturesMap.clear();

		for (const auto& [blockId, blockTexturesNames] : s_BlockTexturesNamesMap)
		{
			BlockTextures blockTextures;
			blockTextures.top = textureAtlas.GetTextureID(blockTexturesNames.top);
			blockTextures.isTopTinted = blockTexturesNames.isTopTinted;
			blockTextures.bottom = textureAtlas.GetTextureID(blockTexturesNames.bottom);
			blockTextures.isBottomTinted = blockTexturesNames.isBottomTinted;
			blockTextures.north = textureAtlas.GetTextureID(blockTexturesNames.north);
			blockTextures.isNorthTinted = blockTexturesNames.isNorthTinted;
			blockTextures.south = textureAtlas.GetTextureID(blockTexturesNames.south);
			blockTextures.isSouthTinted = blockTexturesNames.isSouthTinted;
			blockTextures.east = textureAtlas.GetTextureID(blockTexturesNames.east);
			blockTextures.isEastTinted = blockTexturesNames.isEastTinted;
			blockTextures.west = textureAtlas.GetTextureID(blockTexturesNames.west);
			blockTextures.isWestTinted = blockTexturesNames.isWestTinted;
			blockTextures.overlayTop = textureAtlas.GetTextureID(blockTexturesNames.overlayTop);
			blockTextures.isOverlayTopTinted = blockTexturesNames.isOverlayTopTinted;
			blockTextures.overlayBottom = textureAtlas.GetTextureID(blockTexturesNames.overlayBottom);
			blockTextures.isOverlayBottomTinted = blockTexturesNames.isOverlayBottomTinted;
			blockTextures.overlayNorth = textureAtlas.GetTextureID(blockTexturesNames.overlayNorth);
			blockTextures.isOverlayNorthTinted = blockTexturesNames.isOverlayNorthTinted;
			blockTextures.overlaySouth = textureAtlas.GetTextureID(blockTexturesNames.overlaySouth);
			blockTextures.isOverlaySouthTinted = blockTexturesNames.isOverlaySouthTinted;
			blockTextures.overlayEast = textureAtlas.GetTextureID(blockTexturesNames.overlayEast);
			blockTextures.isOverlayEastTinted = blockTexturesNames.isOverlayEastTinted;
			blockTextures.overlayWest = textureAtlas.GetTextureID(blockTexturesNames.overlayWest);
			blockTextures.isOverlayWestTinted = blockTexturesNames.isOverlayWestTinted;
			s_BlockTexturesMap[blockId] = blockTextures;
		}
	}

	const BlockTexturesRepository::BlockTextures& BlockTexturesRepository::GetBlockTextures(BlockId blockId)
	{
		return s_BlockTexturesMap.at(blockId);
	}

	BlockTexturesRepository::BlockTexturesNames::BlockTexturesNames(const std::string& textureName)
		: top(textureName), bottom(textureName), north(textureName), south(textureName), east(textureName),
		  west(textureName)
	{
	}
	BlockTexturesRepository::BlockTexturesNames::BlockTexturesNames(const std::string& top,
																	bool isTopTinted,
																	const std::string& bottom,
																	bool isBottomTinted,
																	const std::string& north,
																	bool isNorthTinted,
																	const std::string& south,
																	bool isSouthTinted,
																	const std::string& east,
																	bool isEastTinted,
																	const std::string& west,
																	bool isWestTinted)
		: top(top), isTopTinted(isTopTinted), bottom(bottom), isBottomTinted(isBottomTinted), north(north),
		  isNorthTinted(isNorthTinted), south(south), isSouthTinted(isSouthTinted), east(east),
		  isEastTinted(isEastTinted), west(west), isWestTinted(isWestTinted)
	{
	}
	BlockTexturesRepository::BlockTexturesNames::BlockTexturesNames(const std::string& top,
																	bool isTopTinted,
																	const std::string& bottom,
																	bool isBottomTinted,
																	const std::string& north,
																	bool isNorthTinted,
																	const std::string& south,
																	bool isSouthTinted,
																	const std::string& east,
																	bool isEastTinted,
																	const std::string& west,
																	bool isWestTinted,
																	const std::string& overlayTop,
																	bool isOverlayTopTinted,
																	const std::string& overlayBottom,
																	bool isOverlayBottomTinted,
																	const std::string& overlayNorth,
																	bool isOverlayNorthTinted,
																	const std::string& overlaySouth,
																	bool isOverlaySouthTinted,
																	const std::string& overlayEast,
																	bool isOverlayEastTinted,
																	const std::string& overlayWest,
																	bool isOverlayWestTinted)
		: top(top), isTopTinted(isTopTinted), bottom(bottom), isBottomTinted(isBottomTinted), north(north),
		  isNorthTinted(isNorthTinted), south(south), isSouthTinted(isSouthTinted), east(east),
		  isEastTinted(isEastTinted), west(west), isWestTinted(isWestTinted), overlayTop(overlayTop),
		  isOverlayTopTinted(isOverlayTopTinted), overlayBottom(overlayBottom),
		  isOverlayBottomTinted(isOverlayBottomTinted), overlayNorth(overlayNorth),
		  isOverlayNorthTinted(isOverlayNorthTinted), overlaySouth(overlaySouth),
		  isOverlaySouthTinted(isOverlaySouthTinted), overlayEast(overlayEast),
		  isOverlayEastTinted(isOverlayEastTinted), overlayWest(overlayWest), isOverlayWestTinted(isOverlayWestTinted)
	{
	}
} // namespace onion::voxel
