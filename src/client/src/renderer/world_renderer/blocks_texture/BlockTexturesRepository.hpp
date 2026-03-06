#pragma once

#include <cstdint>
#include <unordered_map>

#include <shared/world/block/BlockIds.hpp>

#include "../../texture_atlas/TextureAtlas.hpp"

namespace onion::voxel
{
	class BlockTexturesRepository
	{

		// ----- Structs -----
	  public:
		struct BlockTextures
		{
			TextureAtlas::TextureID top;
			bool isTopTinted = false;

			TextureAtlas::TextureID bottom;
			bool isBottomTinted = false;

			TextureAtlas::TextureID north;
			bool isNorthTinted = false;

			TextureAtlas::TextureID south;
			bool isSouthTinted = false;

			TextureAtlas::TextureID east;
			bool isEastTinted = false;

			TextureAtlas::TextureID west;
			bool isWestTinted = false;

			TextureAtlas::TextureID overlayTop = 0;
			bool isOverlayTopTinted = false;

			TextureAtlas::TextureID overlayBottom = 0;
			bool isOverlayBottomTinted = false;

			TextureAtlas::TextureID overlayNorth = 0;
			bool isOverlayNorthTinted = false;

			TextureAtlas::TextureID overlaySouth = 0;
			bool isOverlaySouthTinted = false;

			TextureAtlas::TextureID overlayEast = 0;
			bool isOverlayEastTinted = false;

			TextureAtlas::TextureID overlayWest = 0;
			bool isOverlayWestTinted = false;
		};

		struct BlockTexturesNames
		{
			std::string top;
			bool isTopTinted = false;

			std::string bottom;
			bool isBottomTinted = false;

			std::string north;
			bool isNorthTinted = false;

			std::string south;
			bool isSouthTinted = false;

			std::string east;
			bool isEastTinted = false;

			std::string west;
			bool isWestTinted = false;

			std::string overlayTop = "";
			bool isOverlayTopTinted = false;

			std::string overlayBottom = "";
			bool isOverlayBottomTinted = false;

			std::string overlayNorth = "";
			bool isOverlayNorthTinted = false;

			std::string overlaySouth = "";
			bool isOverlaySouthTinted = false;

			std::string overlayEast = "";
			bool isOverlayEastTinted = false;

			std::string overlayWest = "";
			bool isOverlayWestTinted = false;

			BlockTexturesNames() = default;
			BlockTexturesNames(const std::string& textureName);
			BlockTexturesNames(const std::string& top,
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
							   bool isWestTinted);

			BlockTexturesNames(const std::string& top,
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
							   bool isOverlayWestTinted);
		};

	  public:
		static void Initialize(const TextureAtlas& textureAtlas);

	  public:
		static const BlockTextures& GetBlockTextures(BlockId blockId);

	  private:
		static const BlockTexturesNames s_Stone;
		static const BlockTexturesNames s_Dirt;
		static const BlockTexturesNames s_Grass;

		static const std::unordered_map<BlockId, BlockTexturesNames> s_BlockTexturesNamesMap;

		static std::unordered_map<BlockId, BlockTextures> s_BlockTexturesMap;

	  private:
	};

} // namespace onion::voxel
