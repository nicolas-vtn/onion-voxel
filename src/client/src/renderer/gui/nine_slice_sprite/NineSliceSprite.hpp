#pragma once

#include "../../Variables.hpp"
#include "../../shader/shader.hpp"
#include "../../texture/texture.hpp"
#include "../GuiElement.hpp"

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
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

	  public:
		NineSliceSprite(const std::filesystem::path& spritePath)
			: GuiElement("NineSliceSprite"), m_PathSprite(spritePath)
		{
			LoadTextures(spritePath);
		}

		void LoadTextures(const std::filesystem::path& spritePath)
		{
			m_PathSprite = spritePath;

			if (!std::filesystem::exists(spritePath))
			{
				std::cerr << "Error: Sprite file does not exist: " << spritePath << std::endl;
				throw std::runtime_error("Sprite file does not exist: " + spritePath.string());
			}

			// Loads the texture
			m_Texture.Delete();
			m_Texture.LoadFromFile(spritePath);

			// Loads Metadata
			m_PathSpriteMetadata = spritePath.parent_path() / (spritePath.filename().string() + ".mcmeta");
			m_NineSliceMetadata = ReadMetadataFromFile(m_PathSpriteMetadata);

			// Get the pixel coordinates of the borders in the original texture
			int x0 = 0;
			int x1 = m_NineSliceMetadata.LeftBorder;
			int x2 = m_Texture.Width() - m_NineSliceMetadata.RightBorder;
			int x3 = m_Texture.Width();

			int y0 = 0;
			int y1 = m_NineSliceMetadata.TopBorder;
			int y2 = m_Texture.Height() - m_NineSliceMetadata.BottomBorder;
			int y3 = m_Texture.Height();

			// Create the 9 sub-textures
			m_TextureTopLeft = std::move(m_Texture.SubTexture(x0, y0, x1 - x0, y1 - y0));
			m_TextureTop = std::move(m_Texture.SubTexture(x1, y0, x2 - x1, y1 - y0));
			m_TextureTopRight = std::move(m_Texture.SubTexture(x2, y0, x3 - x2, y1 - y0));
			m_TextureLeft = std::move(m_Texture.SubTexture(x0, y1, x1 - x0, y2 - y1));
			m_TextureCenter = std::move(m_Texture.SubTexture(x1, y1, x2 - x1, y2 - y1));
			m_TextureRight = std::move(m_Texture.SubTexture(x2, y1, x3 - x2, y2 - y1));
			m_TextureBottomLeft = std::move(m_Texture.SubTexture(x0, y2, x1 - x0, y3 - y2));
			m_TextureBottom = std::move(m_Texture.SubTexture(x1, y2, x2 - x1, y3 - y2));
			m_TextureBottomRight = std::move(m_Texture.SubTexture(x2, y2, x3 - x2, y3 - y2));

			m_MeshDirty = true;
		}

		~NineSliceSprite()
		{
			if (m_VAO_TopLeft || m_VAO_Top || m_VAO_TopRight || m_VAO_Left || m_VAO_Center || m_VAO_Right ||
				m_VBO_BottomLeft || m_VBO_Bottom || m_VBO_BottomRight || m_EBO)
			{
				std::cerr << "Warning: NineSliceSprite destructor called but OpenGL buffers were not deleted. There is "
							 "a memory leak."
						  << std::endl;
			}
		}

		// ----- Public API -----
	  public:
		void Render() override
		{
			if (m_MeshDirty)
			{
				BuildNineSliceMesh();
				UploadMesh();
			}

			//m_Texture.Bind();
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

			s_ShaderNineSliceSprites.Use();
			s_ShaderNineSliceSprites.setInt("uTexture", 0);

			glActiveTexture(GL_TEXTURE0);

			// Top Left Texture
			glBindVertexArray(m_VAO_TopLeft);
			m_TextureTopLeft.Bind();
			glBindBuffer(GL_ARRAY_BUFFER, m_VBO_TopLeft);
			glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_Indices.size()), GL_UNSIGNED_INT, 0);

			// Top Texture
			glBindVertexArray(m_VAO_Top);
			m_TextureTop.Bind();
			glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Top);
			glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_Indices.size()), GL_UNSIGNED_INT, 0);

			glBindVertexArray(0);
		}
		void Initialize() override
		{
			GenerateBuffers();
			InitBuffers();
		}
		void Delete() override { DeleteBuffers(); }

		// ----- Getters / Setters -----
	  public:
		const glm::vec2& GetPosition() const { return m_Position; }
		void SetPosition(const glm::vec2& position)
		{
			m_Position = position;
			m_MeshDirty = true;
		}

		const glm::vec2& GetSize() const { return m_Size; }
		void SetSize(const glm::vec2& size)
		{
			m_Size = size;
			m_MeshDirty = true;
		}

		// ----- Internal Variables -----
	  private:
		std::filesystem::path m_PathSprite;
		std::filesystem::path m_PathSpriteMetadata;

		glm::ivec2 m_Position{0, 0};
		glm::ivec2 m_Size{1, 1};

		bool m_MeshDirty = true;
		glm::ivec2 m_LastBuiltSize{-1, -1};

		Texture m_Texture;
		NineSliceMetadata m_NineSliceMetadata;

		static inline int s_GUI_SCALE = 8;

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

		void GenerateBuffers()
		{
			glGenVertexArrays(1, &m_VAO_TopLeft);
			glGenVertexArrays(1, &m_VAO_Top);
			glGenVertexArrays(1, &m_VAO_TopRight);
			glGenVertexArrays(1, &m_VAO_Left);
			glGenVertexArrays(1, &m_VAO_Center);
			glGenVertexArrays(1, &m_VAO_Right);
			glGenVertexArrays(1, &m_VAO_BottomLeft);
			glGenVertexArrays(1, &m_VAO_Bottom);
			glGenVertexArrays(1, &m_VAO_BottomRight);

			glGenBuffers(1, &m_VBO_TopLeft);
			glGenBuffers(1, &m_VBO_Top);
			glGenBuffers(1, &m_VBO_TopRight);
			glGenBuffers(1, &m_VBO_Left);
			glGenBuffers(1, &m_VBO_Center);
			glGenBuffers(1, &m_VBO_Right);
			glGenBuffers(1, &m_VBO_BottomLeft);
			glGenBuffers(1, &m_VBO_Bottom);
			glGenBuffers(1, &m_VBO_BottomRight);
			glGenBuffers(1, &m_EBO);
		}
		void InitBuffers()
		{
			// ------------------ TOP LEFT ------------------
			glBindVertexArray(m_VAO_TopLeft);
			glBindBuffer(GL_ARRAY_BUFFER, m_VBO_TopLeft);
			glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, x));
			glEnableVertexAttribArray(0);

			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, u));
			glEnableVertexAttribArray(1);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(uint32_t), m_Indices.data(), GL_STATIC_DRAW);

			// ------------------ TOP ------------------
			glBindVertexArray(m_VAO_Top);
			glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Top);
			glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, x));
			glEnableVertexAttribArray(0);

			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, u));
			glEnableVertexAttribArray(1);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(uint32_t), m_Indices.data(), GL_STATIC_DRAW);

			//glBindBuffer(GL_ARRAY_BUFFER, m_VBO_TopRight);
			//glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

			//glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Left);
			//glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

			//glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Center);
			//glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

			//glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Right);
			//glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

			//glBindBuffer(GL_ARRAY_BUFFER, m_VBO_BottomLeft);
			//glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

			//glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Bottom);
			//glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

			//glBindBuffer(GL_ARRAY_BUFFER, m_VBO_BottomRight);
			//glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

			//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
			//glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(uint32_t), m_Indices.data(), GL_STATIC_DRAW);

			glBindVertexArray(0);
		}
		void DeleteBuffers()
		{
			glDeleteVertexArrays(1, &m_VAO_TopLeft);
			glDeleteVertexArrays(1, &m_VAO_Top);
			glDeleteVertexArrays(1, &m_VAO_TopRight);
			glDeleteVertexArrays(1, &m_VAO_Left);
			glDeleteVertexArrays(1, &m_VAO_Center);
			glDeleteVertexArrays(1, &m_VAO_Right);
			glDeleteVertexArrays(1, &m_VAO_BottomLeft);
			glDeleteVertexArrays(1, &m_VAO_Bottom);
			glDeleteVertexArrays(1, &m_VAO_BottomRight);

			glDeleteBuffers(1, &m_VBO_TopLeft);
			glDeleteBuffers(1, &m_VBO_Top);
			glDeleteBuffers(1, &m_VBO_TopRight);
			glDeleteBuffers(1, &m_VBO_Left);
			glDeleteBuffers(1, &m_VBO_Center);
			glDeleteBuffers(1, &m_VBO_Right);
			glDeleteBuffers(1, &m_VBO_BottomLeft);
			glDeleteBuffers(1, &m_VBO_Bottom);
			glDeleteBuffers(1, &m_VBO_BottomRight);

			glDeleteBuffers(1, &m_EBO);

			m_VAO_TopLeft = 0;
			m_VAO_Top = 0;
			m_VAO_TopRight = 0;
			m_VAO_Left = 0;
			m_VAO_Center = 0;
			m_VAO_Right = 0;
			m_VAO_BottomLeft = 0;
			m_VAO_Bottom = 0;
			m_VAO_BottomRight = 0;

			m_VBO_TopLeft = 0;
			m_VBO_Top = 0;
			m_VBO_TopRight = 0;
			m_VBO_Left = 0;
			m_VBO_Center = 0;
			m_VBO_Right = 0;
			m_VBO_BottomLeft = 0;
			m_VBO_Bottom = 0;
			m_VBO_BottomRight = 0;

			m_EBO = 0;
		}
		void UploadMesh()
		{
			// ------------------- TOP LEFT ------------------
			glBindVertexArray(m_VAO_TopLeft);

			glBindBuffer(GL_ARRAY_BUFFER, m_VBO_TopLeft);
			glBufferData(GL_ARRAY_BUFFER,
						 m_Vertices_TopLeft.size() * sizeof(Vertex),
						 m_Vertices_TopLeft.data(),
						 GL_DYNAMIC_DRAW);

			// ------------------- TOP ------------------
			glBindVertexArray(m_VAO_Top);

			glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Top);
			glBufferData(
				GL_ARRAY_BUFFER, m_Vertices_Top.size() * sizeof(Vertex), m_Vertices_Top.data(), GL_DYNAMIC_DRAW);

			// ------------------- TOP RIGHT ------------------
			glBindVertexArray(m_VAO_TopRight);
			glBindBuffer(GL_ARRAY_BUFFER, m_VBO_TopRight);
			glBufferData(GL_ARRAY_BUFFER,
						 m_Vertices_TopRight.size() * sizeof(Vertex),
						 m_Vertices_TopRight.data(),
						 GL_DYNAMIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Left);
			glBufferData(
				GL_ARRAY_BUFFER, m_Vertices_Left.size() * sizeof(Vertex), m_Vertices_Left.data(), GL_DYNAMIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Center);
			glBufferData(
				GL_ARRAY_BUFFER, m_Vertices_Center.size() * sizeof(Vertex), m_Vertices_Center.data(), GL_DYNAMIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Right);
			glBufferData(
				GL_ARRAY_BUFFER, m_Vertices_Right.size() * sizeof(Vertex), m_Vertices_Right.data(), GL_DYNAMIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, m_VBO_BottomLeft);
			glBufferData(GL_ARRAY_BUFFER,
						 m_Vertices_BottomLeft.size() * sizeof(Vertex),
						 m_Vertices_BottomLeft.data(),
						 GL_DYNAMIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Bottom);
			glBufferData(
				GL_ARRAY_BUFFER, m_Vertices_Bottom.size() * sizeof(Vertex), m_Vertices_Bottom.data(), GL_DYNAMIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, m_VBO_BottomRight);
			glBufferData(GL_ARRAY_BUFFER,
						 m_Vertices_BottomRight.size() * sizeof(Vertex),
						 m_Vertices_BottomRight.data(),
						 GL_DYNAMIC_DRAW);

			glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Top);
			glBufferData(
				GL_ARRAY_BUFFER, m_Vertices_Top.size() * sizeof(Vertex), m_Vertices_Top.data(), GL_DYNAMIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
			glBufferData(
				GL_ELEMENT_ARRAY_BUFFER, m_Indices.size() * sizeof(uint32_t), m_Indices.data(), GL_STATIC_DRAW);
		}

		// ----- Helpers -----
	  private:
		inline static NineSliceMetadata ReadMetadataFromFile(const std::filesystem::path& pathMetadataFile)
		{
			if (!std::filesystem::exists(pathMetadataFile))
			{
				std::cerr << "Error: Sprite metadata file does not exist: " << pathMetadataFile << std::endl;
				throw std::runtime_error("Sprite metadata file does not exist: " + pathMetadataFile.string());
			}

			std::ifstream file(pathMetadataFile);
			if (!file.is_open())
			{
				throw std::runtime_error("Failed to open metadata file: " + pathMetadataFile.string());
			}

			nlohmann::json json;
			file >> json;

			try
			{
				const auto& scaling = json.at("gui").at("scaling");

				if (scaling.at("type") != "nine_slice")
				{
					throw std::runtime_error("Scaling type is not 'nine_slice'");
				}

				NineSliceMetadata metadata;
				metadata.Width = scaling.at("width").get<int>();
				metadata.Height = scaling.at("height").get<int>();

				const auto& border = scaling.at("border");

				// Case 1: uniform border (integer)
				if (border.is_number_integer())
				{
					int value = border.get<int>();
					metadata.LeftBorder = value;
					metadata.RightBorder = value;
					metadata.TopBorder = value;
					metadata.BottomBorder = value;
				}
				// Case 2: per-side border (object)
				else if (border.is_object())
				{
					metadata.LeftBorder = border.value("left", 0);
					metadata.RightBorder = border.value("right", 0);
					metadata.TopBorder = border.value("top", 0);
					metadata.BottomBorder = border.value("bottom", 0);
				}
				else
				{
					throw std::runtime_error("Invalid border format in metadata");
				}

				return metadata;
			}
			catch (const std::exception& e)
			{
				throw std::runtime_error("Invalid nine-slice metadata format in file: " + pathMetadataFile.string() +
										 " | Reason: " + e.what());
			}
		}

		void BuildNineSliceMesh()
		{
			BuildVertices();

			// Indices are constant; build once
			if (m_Indices.empty())
			{
				BuildIndices();
			}

			m_LastBuiltSize = m_Size;
			m_MeshDirty = false;
		}

		void BuildVertices()
		{
			const auto& meta = m_NineSliceMetadata;

			// If size is invalid, don't build
			if (m_Size.x <= 0.0f || m_Size.y <= 0.0f)
				return;

			int leftBorderPx = meta.LeftBorder * s_GUI_SCALE;
			int rightBorderPx = meta.RightBorder * s_GUI_SCALE;
			int topBorderPx = meta.TopBorder * s_GUI_SCALE;
			int bottomBorderPx = meta.BottomBorder * s_GUI_SCALE;

			int centerWidth = std::max(0, m_Size.x - leftBorderPx - rightBorderPx);
			int centerHeight = std::max(0, m_Size.y - topBorderPx - bottomBorderPx);

			// Geometry (in Pixel Coordinates relative to the top-left corner of the sprite)
			int x0 = 0;
			int x1 = leftBorderPx;
			int x2 = leftBorderPx + centerWidth;
			int x3 = leftBorderPx + centerWidth + rightBorderPx;

			int y0 = 0;
			int y1 = topBorderPx;
			int y2 = topBorderPx + centerHeight;
			int y3 = topBorderPx + centerHeight + bottomBorderPx;

			// Convert from local top-left space to world pixel space using topLeft
			glm::ivec2 topLeft = m_Position - glm::ivec2(std::ceil(m_Size.x * 0.5f), std::ceil(m_Size.y * 0.5f));

			float xs[4] = {x0 + topLeft.x, x1 + topLeft.x, x2 + topLeft.x, x3 + topLeft.x};
			float ys[4] = {y0 + topLeft.y, y1 + topLeft.y, y2 + topLeft.y, y3 + topLeft.y};

			// TopLeft Texture (no repeat / no crop)
			m_Vertices_TopLeft = {
				{xs[0], ys[0], 0.0f, 0.0f, 0.0f}, // Top Left
				{xs[1], ys[0], 0.0f, 1.0f, 0.0f}, // Top Right
				{xs[1], ys[1], 0.0f, 1.0f, 1.0f}, // Bottom Right
				{xs[0], ys[1], 0.0f, 0.0f, 1.0f}  // Bottom Left
			};

			// Top Texture (repeat horizontally, no crop vertically)
			float u0 = 0.0f;
			float u1 = static_cast<float>(centerWidth) / meta.Width / s_GUI_SCALE;
			float v0 = 0.0f;
			float v1 = 1.0f;
			m_Vertices_Top = {
				{xs[1], ys[0], 0.0f, u0, v0}, // Top Left
				{xs[2], ys[0], 0.0f, u1, v0}, // Top Right
				{xs[2], ys[1], 0.0f, u1, v1}, // Bottom Right
				{xs[1], ys[1], 0.0f, u0, v1}  // Bottom Left
			};
		}

		void BuildIndices() { m_Indices = {0, 1, 2, 2, 3, 0}; }
	};

} // namespace onion::voxel
