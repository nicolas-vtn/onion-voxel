#pragma once

#include "../../Variables.hpp"
#include "../../shader/shader.hpp"
#include "../../texture/texture.hpp"
#include "../GuiElement.hpp"

#include <onion/Event.hpp>

#include <glm/glm.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace onion::voxel
{

	class NineSliceSprite : public GuiElement
	{
	  private:
		struct NineSliceMetadata
		{
			int Width = 0;
			int Height = 0;
			int LeftBorder = 0;
			int RightBorder = 0;
			int TopBorder = 0;
			int BottomBorder = 0;
		};

		struct Vertex
		{
			float x, y, z;
			float u, v;
		};

		// ----- Constructor & Destructor -----
	  public:
		NineSliceSprite() = delete;
		NineSliceSprite(const std::string& name, const std::filesystem::path& spritePath);
		~NineSliceSprite();

		// ----- Public API -----
	  public:
		void LoadTextures(const std::filesystem::path& spritePath);

		void Initialize() override;
		void Render() override;
		void Delete() override;

		void PullEvents();

		bool IsHovered() const;

		// ----- Events -----
	  public:
		Event<const NineSliceSprite&> OnMouseDown;
		Event<const NineSliceSprite&> OnMouseUp;
		Event<const NineSliceSprite&> OnClick;

		Event<const NineSliceSprite&> OnHoverEnter;
		Event<const NineSliceSprite&> OnHoverLeave;

		// ----- Getters / Setters -----
	  public:
		const glm::ivec2& GetPosition() const;
		void SetPosition(const glm::ivec2& position);

		const glm::ivec2& GetSize() const;
		void SetSize(const glm::ivec2& size);

		// ----- Internal Variables -----
	  private:
		std::filesystem::path m_PathSprite;
		std::filesystem::path m_PathSpriteMetadata;

		glm::ivec2 m_Position{0, 0};
		glm::ivec2 m_Size{1, 1};

		glm::ivec2 m_LastBuiltSize{-1, -1};
		glm::ivec2 m_LastBuiltPosition{-1, -1};
		int m_LastBuiltGuiScale = -1;

		bool m_MeshDirty = true;

		NineSliceMetadata m_NineSliceMetadata;

		// ----- Event State -----
	  private:
		bool m_WasHovered = false;
		bool m_WasMouseDown = false;

		// ----- Nine Slice Textures -----
	  private:
		Texture m_TextureTopLeft;
		Texture m_TextureTop;
		Texture m_TextureTopRight;
		Texture m_TextureLeft;
		Texture m_TextureCenter;
		Texture m_TextureRight;
		Texture m_TextureBottomLeft;
		Texture m_TextureBottom;
		Texture m_TextureBottomRight;

		// ----- Open GL -----
	  private:
		std::vector<Vertex> m_Vertices_TopLeft{};
		std::vector<Vertex> m_Vertices_Top{};
		std::vector<Vertex> m_Vertices_TopRight{};
		std::vector<Vertex> m_Vertices_Left{};
		std::vector<Vertex> m_Vertices_Center{};
		std::vector<Vertex> m_Vertices_Right{};
		std::vector<Vertex> m_Vertices_BottomLeft{};
		std::vector<Vertex> m_Vertices_Bottom{};
		std::vector<Vertex> m_Vertices_BottomRight{};

		std::vector<uint32_t> m_Indices{};

	  private:
		GLuint m_VAO_TopLeft = 0;
		GLuint m_VBO_TopLeft = 0;

		GLuint m_VAO_Top = 0;
		GLuint m_VBO_Top = 0;

		GLuint m_VAO_TopRight = 0;
		GLuint m_VBO_TopRight = 0;

		GLuint m_VAO_Left = 0;
		GLuint m_VBO_Left = 0;

		GLuint m_VAO_Center = 0;
		GLuint m_VBO_Center = 0;

		GLuint m_VAO_Right = 0;
		GLuint m_VBO_Right = 0;

		GLuint m_VAO_BottomLeft = 0;
		GLuint m_VBO_BottomLeft = 0;

		GLuint m_VAO_Bottom = 0;
		GLuint m_VBO_Bottom = 0;

		GLuint m_VAO_BottomRight = 0;
		GLuint m_VBO_BottomRight = 0;

		GLuint m_EBO = 0;

		void GenerateBuffers();
		void InitBuffers();
		void DeleteBuffers();

		void UploadMesh();

		// ----- Helpers -----
	  private:
		inline static NineSliceMetadata ReadMetadataFromFile(const std::filesystem::path& pathMetadataFile);

		void BuildNineSliceMesh();
		void BuildVertices();
		void BuildIndices();
	};

} // namespace onion::voxel
