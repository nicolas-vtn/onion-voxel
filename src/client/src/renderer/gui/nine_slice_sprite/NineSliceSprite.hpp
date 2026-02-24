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
			LoadTexture(spritePath);
		}

		void LoadTexture(const std::filesystem::path& spritePath)
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

			m_NineSliceMetadata.Height = m_Texture.Height();
			m_NineSliceMetadata.Width = m_Texture.Width();

			m_MeshDirty = true;
		}

		~NineSliceSprite()
		{
			if (m_VAO || m_VBO || m_EBO)
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

			m_Texture.Bind();
			s_ShaderNineSliceSprites.Use();
			s_ShaderNineSliceSprites.setInt("uTexture", 0);

			glActiveTexture(GL_TEXTURE0);
			glBindVertexArray(m_VAO);
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

		glm::vec2 m_Position{0, 0};
		glm::vec2 m_Size{1, 1};

		bool m_MeshDirty = true;
		glm::vec2 m_LastBuiltSize{-1.0f, -1.0f};

		Texture m_Texture;
		NineSliceMetadata m_NineSliceMetadata;

		// ----- Open GL -----
	  private:
		std::vector<Vertex> m_Vertices{};
		std::vector<uint32_t> m_Indices{};

	  private:
		GLuint m_VAO = 0;
		GLuint m_VBO = 0;
		GLuint m_EBO = 0;

		void GenerateBuffers()
		{
			glGenVertexArrays(1, &m_VAO);
			glGenBuffers(1, &m_VBO);
			glGenBuffers(1, &m_EBO);
		}
		void InitBuffers()
		{
			glBindVertexArray(m_VAO);

			glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
			glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
			glBufferData(
				GL_ELEMENT_ARRAY_BUFFER, m_Indices.size() * sizeof(uint32_t), m_Indices.data(), GL_STATIC_DRAW);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, x));
			glEnableVertexAttribArray(0);

			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, u));
			glEnableVertexAttribArray(1);

			glBindVertexArray(0);
		}
		void DeleteBuffers()
		{
			glDeleteVertexArrays(1, &m_VAO);
			glDeleteBuffers(1, &m_VBO);
			glDeleteBuffers(1, &m_EBO);

			m_VAO = 0;
			m_VBO = 0;
			m_EBO = 0;
		}
		void UploadMesh()
		{
			glBindVertexArray(m_VAO);

			glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
			glBufferData(GL_ARRAY_BUFFER, m_Vertices.size() * sizeof(Vertex), m_Vertices.data(), GL_DYNAMIC_DRAW);

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
			const auto& meta = m_NineSliceMetadata;

			// If size is invalid, don't build
			if (m_Size.x <= 0.0f || m_Size.y <= 0.0f)
				return;

			const float scaleX = m_Size.x / static_cast<float>(meta.Width);
			const float scaleY = m_Size.y / static_cast<float>(meta.Height);

			// Uniform scale
			const float scale = std::max(scaleX, scaleY);

			float L = meta.LeftBorder * scale;
			float R = meta.RightBorder * scale;
			float T = meta.TopBorder * scale;
			float B = meta.BottomBorder * scale;

			// Clamp so borders never exceed half the final size
			L = std::min(L, m_Size.x * 0.5f);
			R = std::min(R, m_Size.x * 0.5f);
			T = std::min(T, m_Size.y * 0.5f);
			B = std::min(B, m_Size.y * 0.5f);

			// Geometry (pixel space) from top-left origin
			// We'll build in local space (0..width, 0..height), then add topLeft in CPU.
			const float x0 = 0.0f;
			const float x1 = L;
			const float x2 = m_Size.x - R;
			const float x3 = m_Size.x;

			const float y0 = 0.0f;
			const float y1 = T;
			const float y2 = m_Size.y - B;
			const float y3 = m_Size.y;

			// UV splits are based on the SOURCE texture (metadata width/height)
			const float u0 = 0.0f;
			const float u1 = static_cast<float>(meta.LeftBorder) / static_cast<float>(meta.Width);
			const float u2 = 1.0f - static_cast<float>(meta.RightBorder) / static_cast<float>(meta.Width);
			const float u3 = 1.0f;

			const float v0 = 0.0f;
			const float v1 = static_cast<float>(meta.TopBorder) / static_cast<float>(meta.Height);
			const float v2 = 1.0f - static_cast<float>(meta.BottomBorder) / static_cast<float>(meta.Height);
			const float v3 = 1.0f;

			const float xs[4] = {x0, x1, x2, x3};
			const float ys[4] = {y0, y1, y2, y3};
			const float us[4] = {u0, u1, u2, u3};
			const float vs[4] = {v0, v1, v2, v3};

			// Convert from local top-left space to world pixel space using topLeft
			glm::vec2 topLeft = m_Position - m_Size * 0.5f;

			// snap top-left to pixel grid
			topLeft.x = std::round(topLeft.x);
			topLeft.y = std::round(topLeft.y);

			m_Vertices.clear();
			m_Vertices.reserve(16);

			for (int j = 0; j < 4; ++j)
			{
				for (int i = 0; i < 4; ++i)
				{
					Vertex v{};
					v.x = topLeft.x + xs[i];
					v.y = topLeft.y + ys[j];
					v.z = 0.0f;

					v.u = us[i];
					v.v = vs[j];

					m_Vertices.push_back(v);
				}
			}

			// Indices are constant; build once
			if (m_Indices.empty())
			{
				m_Indices.reserve(54);
				for (int row = 0; row < 3; ++row)
				{
					for (int col = 0; col < 3; ++col)
					{
						uint32_t tl = row * 4 + col;
						uint32_t tr = tl + 1;
						uint32_t bl = (row + 1) * 4 + col;
						uint32_t br = bl + 1;

						m_Indices.push_back(tl);
						m_Indices.push_back(bl);
						m_Indices.push_back(tr);
						m_Indices.push_back(tr);
						m_Indices.push_back(bl);
						m_Indices.push_back(br);
					}
				}
			}

			m_LastBuiltSize = m_Size;
			m_MeshDirty = false;
		}
	};

} // namespace onion::voxel
