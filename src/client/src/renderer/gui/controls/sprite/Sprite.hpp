#pragma once

#include "../../../texture/texture.hpp"
#include "../../GuiElement.hpp"

#include <Event.hpp>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace onion::voxel
{
	class Sprite : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		Sprite(const std::string& name, const std::string& spritePath);
		~Sprite();

		// ----- Public API -----
	  public:
		void Render() override;
		void Initialize() override;
		void Delete() override;

		// ----- Getters / Setters -----
	  public:
		void SetSize(const glm::vec2& size);
		glm::vec2 GetSize() const;

		void SetPosition(const glm::vec2& pos);
		glm::vec2 GetPosition() const;

		int GetTextureWidth() const;
		int GetTextureHeight() const;

		// ----- Properties -----
	  public:
		glm::vec2 m_Position{0, 0};
		glm::vec2 m_Size{1, 1};

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
