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
		// ----- Structs -----
	  public:
		using TextureID = uint16_t;

		struct AtlasEntry
		{
			glm::vec2 uvMin;
			glm::vec2 uvMax;
		};

		// ----- Constructor / Destructor -----
	  public:
		TextureAtlas(const std::filesystem::path& directory);

		// ----- Public API -----
	  public:
		void Bind() const;

		void Unload();

		// ----- Getters / Setters -----
	  public:
		TextureID GetTextureID(const std::string& name) const;
		const AtlasEntry& GetAtlasEntry(TextureID id) const;

		// ----- Private Methods -----
	  private:
		void ScanTextures();
		void BuildAtlas();

		// ----- Private Members -----
	  private:
		std::filesystem::path m_Directory;

		std::vector<std::filesystem::path> m_TextureFiles;

		std::unordered_map<std::string, TextureID> m_NameToID;

		std::vector<AtlasEntry> m_Entries;

		// ----- Texture Atlas Info -----
	  private:
		Texture m_Texture;

		int m_TextureSize = 16;
		int m_AtlasSize = 0;
	};

} // namespace onion::voxel
