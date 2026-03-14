#pragma once

#include <glm/glm.hpp>

#include <string>
#include <vector>

#include <onion/Event.hpp>

#include <renderer/gui/GuiElement.hpp>
#include <renderer/texture/texture.hpp>

#include <renderer/OpenGL.hpp>

namespace onion::voxel
{
	class Sprite : public GuiElement
	{
		// ----- Enums -----
	  public:
		enum class eOrigin
		{
			Asset,
			ResourcePack
		};

		// ----- Constructor / Destructor -----
	  public:
		Sprite(const std::string& name, const std::filesystem::path& spritePath, eOrigin origin);
		Sprite(const std::string& name, Texture texture);
		~Sprite();

		// ----- Public API -----
	  public:
		void Render() override;
		void Initialize() override;
		void Delete() override;
		void ReloadTextures() override;

		// ----- Getters / Setters -----
	  public:
		void SetSize(const glm::vec2& size);
		glm::vec2 GetSize() const;

		void SetPosition(const glm::vec2& pos);
		glm::vec2 GetPosition() const;

		int GetTextureWidth() const;
		int GetTextureHeight() const;

		void SetOrigin(eOrigin origin);
		eOrigin GetOrigin() const;

		void SwapTexture(Texture newTexture);

		// ----- Properties -----
	  public:
		std::filesystem::path m_SpritePath;
		glm::vec2 m_Position{0, 0};
		glm::vec2 m_Size{1, 1};
		eOrigin m_Origin = eOrigin::Asset;
		bool m_UnreloadableTexture = false;

		// ----- Texture -----
	  private:
		Texture m_Texture;

		// ----- OPEN GL -----
	  private:
		struct Vertex
		{
			float posX, posY, posZ;
			float texX, texY;
		};

		static std::vector<Vertex> s_Vertices;
		static std::vector<unsigned int> s_Indices;

		GLuint m_VAO = 0;
		GLuint m_VBO = 0;
		GLuint m_EBO = 0;

		void GenerateBuffers();
		void InitBuffers();
		void DeleteBuffers();
	};
}; // namespace onion::voxel
