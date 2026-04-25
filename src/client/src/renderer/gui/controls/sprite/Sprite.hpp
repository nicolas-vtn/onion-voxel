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

		void PullEvents();
		bool IsHovered() const;

		// ----- Getters / Setters -----
	  public:
		void SetSize(const glm::vec2& size);
		glm::vec2 GetSize() const;

		void SetPosition(const glm::vec2& pos);
		glm::vec2 GetPosition() const;

		int GetTextureWidth() const;
		int GetTextureHeight() const;

		/// @brief Sets the offset of the text in the Z direction. In range [-1, 1], where 1 is the closest to the camera and -1 is the farthest from the camera.
		/// @param zOffset The new offset of the text in the Z direction.
		void SetZOffset(float zOffset);
		float GetZOffset() const;

		void SetOrigin(eOrigin origin);
		eOrigin GetOrigin() const;

		void SetCissors(const glm::ivec2& topLeft, const glm::ivec2& bottomRight);

		void SwapTexture(Texture newTexture);

		// ----- Events -----
	  public:
		Event<const Sprite&> EvtClick;
		Event<const Sprite&> EvtHoverEnter;
		Event<const Sprite&> EvtHoverLeave;

		// ----- Properties -----
	  private:
		std::filesystem::path m_SpritePath;
		glm::vec2 m_Position{0, 0};
		glm::vec2 m_Size{1, 1};
		eOrigin m_Origin = eOrigin::Asset;
		bool m_UnreloadableTexture = false;
		float m_Zoffset = 0.f;

		glm::ivec2 m_CissorsTopLeft{0, 0};
		glm::ivec2 m_CissorsBottomRight{0, 0};

		bool m_WasHovered = false;
		bool m_WasMouseDown = false;

		// ----- Texture -----
	  private:
		Texture m_Texture;

		// ----- Private Helpers -----
	  private:
		void StartCissors() const;
		void EndCissors() const;

		// ----- OPEN GL -----
	  private:
		struct Vertex
		{
			float posX, posY;
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
