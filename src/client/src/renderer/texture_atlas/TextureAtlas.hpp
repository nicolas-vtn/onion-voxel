#pragma once

#include <glm/glm.hpp>

#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <renderer/EngineContext.hpp>
#include <renderer/texture/texture.hpp>

namespace onion::voxel
{
	enum class Transparency : uint8_t
	{
		Opaque,
		Cutout,
		Transparent
	};

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
		TextureAtlas();

		// ----- Public API -----
	  public:
		void Bind() const;

		void Initialize(const std::unordered_set<std::string>& textureNames);
		void ReloadTextures(const std::unordered_set<std::string>& textureNames);

		void Unload();

		// ----- Getters / Setters -----
	  public:
		TextureID GetTextureID(const std::string& name) const;
		const AtlasEntry& GetAtlasEntry(TextureID id) const;
		Transparency GetTextureTransparency(const std::string& name) const;

		// ----- Private Methods -----
	  private:
		void BuildAtlas(const std::unordered_set<std::string>& textureNames);

		// ----- Private Helpers -----
	  private:
		static Transparency GetTextureTransparency(const unsigned char* pixels, int width, int height, int channels);

		// ----- Private Members -----
	  private:
		std::unordered_map<std::string, TextureID> m_NameToID;
		std::unordered_map<std::string, Transparency> m_NameToTransparency;
		std::vector<AtlasEntry> m_Entries;

		// ----- Texture Atlas Info -----
	  private:
		Texture m_Texture;

		int m_TextureSize = 16;
		int m_AtlasSize = 0;

		// ----- Constants -----
	  private:
		static inline const std::filesystem::path s_BlockDirectory =
			std::filesystem::path("assets") / "minecraft" / "textures" / "block";
	};

} // namespace onion::voxel
