#pragma once

#include "../../../texture/texture.hpp"
#include "../../GuiElement.hpp"
#include "../../nine_slice_sprite/NineSliceSprite.hpp"

#include <onion/Event.hpp>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace onion::voxel
{
	class Button : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		Button(const std::string& name);
		~Button();

		// ----- Public API -----
	  public:
		void Render();

		void Initialize();
		void Delete();

		// ----- Getters / Setters -----
	  public:
		void SetText(const std::string& text);
		std::string GetText() const;

		void SetSize(const glm::vec2& size);
		glm::vec2 GetSize() const;

		void SetPosition(double posX, double posY);
		void SetPosition(const glm::vec2& pos);
		glm::vec2 GetPosition() const;

		bool IsEnabled() const;
		void SetEnabled(bool enabled);

		void SetScaleUpOnHover(bool scaleUp);

		// ----- Events -----
	  public:
		Event<const Button&> OnClick;
		Event<const Button&> OnHover;
		Event<const Button&> OnUnhover;

		// ----- Internal Helpers -----
	  private:
		bool IsHovered() const;

		// ----- Properties -----
	  private:
		std::string m_Text;
		bool m_IsEnabled = true;
		bool m_ScaleUpOnHover = true;

		glm::vec2 m_Position{0, 0};
		glm::vec2 m_Size{1, 1};

		// ----- Textures -----
	  private:
		static Texture s_Texture;
		static Texture s_TextureDisabled;
		static Texture s_TextureHighlighted;

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
		void DeleteBuffers();
		void InitBuffers();

		// ----- Internal States -----
		bool m_WasHovered = false;
		bool m_WasClicked = false;

		// ----- NineSliceTests -----
	  private:
		NineSliceSprite m_NineSliceSprite;
	};
} // namespace onion::voxel
