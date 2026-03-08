#pragma once

#include <glm/glm.hpp>

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include "../texture/texture.hpp"

namespace onion::voxel
{

	class TextureAtlas
	{
	  public:
		using TextureID = uint16_t;

		struct AtlasEntry
		{
			glm::vec2 uvMin;
			glm::vec2 uvMax;
		};

	  public:
		TextureAtlas(const std::filesystem::path& directory);

		void Bind() const;

	  public:
		TextureID GetTextureID(const std::string& name) const;

		const AtlasEntry& GetAtlasEntry(TextureID id) const;

	  private:
		void ScanTextures();
		void BuildAtlas();

	  private:
		std::filesystem::path m_Directory;

		std::vector<std::filesystem::path> m_TextureFiles;

		std::unordered_map<std::string, TextureID> m_NameToID;

		std::vector<AtlasEntry> m_Entries;

	  private:
		Texture m_Texture;

		int m_TextureSize = 16;
		int m_AtlasSize = 0;
	};

} // namespace onion::voxel
