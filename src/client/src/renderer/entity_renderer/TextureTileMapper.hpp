#pragma once

#include <iostream>
#include <unordered_map>

#include <glm/glm.hpp>

namespace onion::voxel
{

	struct TextureTile
	{
		glm::vec2 TopLeft{0, 0};
		glm::vec2 TopRight{0, 0};
		glm::vec2 BottomLeft{0, 0};
		glm::vec2 BottomRight{0, 0};

		TextureTile() = default;

		TextureTile(const glm::ivec2& bottomLeft_px,
					const glm::ivec2& topRight_px,
					int textureWidth,
					int textureHeight,
					bool flipY = false)
		{
			const float invW = 1.0f / static_cast<float>(textureWidth);
			const float invH = 1.0f / static_cast<float>(textureHeight);

			auto toUV = [&](const glm::ivec2& p) -> glm::vec2
			{
				float u = static_cast<float>(p.x) * invW;
				float v = static_cast<float>(p.y) * invH;
				if (flipY)
					v = 1.0f - v; // convert from top-left origin to bottom-left
				return {u, v};
			};

			const glm::vec2 bl = toUV(bottomLeft_px);
			const glm::vec2 tr = toUV(topRight_px);

			BottomLeft = bl;
			TopRight = tr;
			TopLeft = {bl.x, tr.y};
			BottomRight = {tr.x, bl.y};
		}
	};

	class TextureTileMapper
	{
	  public:
		TextureTileMapper();
		~TextureTileMapper();

		void SetTextureDimensions(int width, int height);

		void AddTextureTile(int textureId, glm::ivec2 bottomLeft_px, glm::ivec2 topRight_px);
		void
		AddTextureTile(int textureId, int bottomLeft_X_px, int bottomLeft_Y_px, int topRight_X_px, int topRight_Y_px);

		void SetFlipY(bool flip);

		const TextureTile& GetTextureTile(int textureId) const;

	  private:
		int m_TextureWidth = -1;
		int m_TextureHeight = -1;

		bool m_FlipY = false;

		std::unordered_map<int, TextureTile> m_TextureTiles; // Map of texture ID to texture tile
	};

} // namespace onion::voxel
