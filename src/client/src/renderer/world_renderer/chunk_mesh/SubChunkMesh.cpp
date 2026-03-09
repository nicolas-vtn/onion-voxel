#include "SubChunkMesh.hpp"

#include "../../Variables.hpp"

namespace onion::voxel
{
	// ----- Static Initialization -----
	Shader SubChunkMesh::s_Shader{
		GetAssetsPath() / "shaders" / "blocks.vert",
		GetAssetsPath() / "shaders" / "blocks.frag",
	};

	Texture SubChunkMesh::s_TextureAtlas;

	// ----- Constructor / Destructor -----

	SubChunkMesh::SubChunkMesh() {}

	SubChunkMesh::~SubChunkMesh()
	{
		if (VBO_Cutout != 0 || VAO_Cutout != 0 || EBO_Cutout != 0 || VBO_Opaque != 0 || VAO_Opaque != 0 ||
			EBO_Opaque != 0 || VBO_Transparent != 0 || VAO_Transparent != 0 || EBO_Transparent != 0)
		{
			std::cerr << "Warning: SubChunkMesh destructor called but OpenGL buffers were not cleaned up. There is a "
						 "memory leak."
					  << std::endl;
		}
	}

	void SubChunkMesh::RenderOpaque()
	{
		if (m_NeedsToPrepareRendering)
		{
			PrepareForRendering();
		}

		if (m_IndicesOpaqueCount > 0)
		{
			glBindVertexArray(VAO_Opaque);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_Opaque);
			glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_IndicesOpaqueCount), GL_UNSIGNED_SHORT, 0);
			glBindVertexArray(0);
		}
	}

	void SubChunkMesh::RenderCutout()
	{
		if (m_IndicesCutoutCount > 0)
		{
			glBindVertexArray(VAO_Cutout);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_Cutout);
			glDrawElements(GL_TRIANGLES, (GLsizei) m_IndicesCutoutCount, GL_UNSIGNED_SHORT, 0);
			glBindVertexArray(0);
		}
	}

	void SubChunkMesh::RenderTransparent()
	{
		if (m_IndicesTransparentCount > 0)
		{
			glBindVertexArray(VAO_Transparent);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_Transparent);
			glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_IndicesTransparentCount), GL_UNSIGNED_SHORT, 0);
			glBindVertexArray(0);
		}
	}

	void SubChunkMesh::Delete()
	{
		CleanupOpenGlBuffers();
	}

	uint32_t SubChunkMesh::GetVertexCount() const
	{
		return m_VertexCount;
	}

	bool SubChunkMesh::IsDirty() const
	{
		return m_IsDirty;
	}

	void SubChunkMesh::SetDirty(bool isDirty)
	{
		m_IsDirty = isDirty;
	}

	void SubChunkMesh::SetLightColor(const glm::vec3& lightColor)
	{
		s_LightColor = lightColor;
		s_Shader.Use();
		s_Shader.setVec3("u_LightColor", lightColor);
	}

	glm::vec3 SubChunkMesh::GetLightColor()
	{
		return s_LightColor;
	}

	void SubChunkMesh::SetUseFaceShading(bool useFaceShading)
	{
		s_UseFaceShading = useFaceShading;
		s_Shader.Use();
		s_Shader.setBool("u_UseFaceShading", useFaceShading);
	}

	bool SubChunkMesh::GetUseFaceShading()
	{
		return s_UseFaceShading;
	}

	void SubChunkMesh::SetUseOcclusion(bool useOcclusion)
	{
		s_UseOcclusion = useOcclusion;
		s_Shader.Use();
		s_Shader.setBool("u_UseOcclusion", useOcclusion);
	}

	bool SubChunkMesh::GetUseOcclusion()
	{
		return s_UseOcclusion;
	}

	void SubChunkMesh::BuffersUpdated()
	{
		m_AreBuffersDataUpToDate = false;
		m_NeedsToPrepareRendering = true;
	}

	void SubChunkMesh::InitOpenGlBuffers()
	{
		CleanupOpenGlBuffers();

		glGenVertexArrays(1, &VAO_Opaque);
		glGenBuffers(1, &VBO_Opaque);
		glGenBuffers(1, &EBO_Opaque);

		glGenVertexArrays(1, &VAO_Cutout);
		glGenBuffers(1, &VBO_Cutout);
		glGenBuffers(1, &EBO_Cutout);

		glGenVertexArrays(1, &VAO_Transparent);
		glGenBuffers(1, &VBO_Transparent);
		glGenBuffers(1, &EBO_Transparent);

		m_AreBuffersGenerated = true;
	}

	void SubChunkMesh::UpdateOpenGlBuffers()
	{
		std::unique_lock lock(m_Mutex);

		m_IndicesOpaqueCount = static_cast<unsigned int>(m_IndicesOpaque.size());
		if (m_VerticesOpaque.size() != 0)
		{

			// bind VAO
			glBindVertexArray(VAO_Opaque);

			// bind VBO & fill
			glBindBuffer(GL_ARRAY_BUFFER, VBO_Opaque);
			glBufferData(
				GL_ARRAY_BUFFER, m_VerticesOpaque.size() * sizeof(Vertex), &m_VerticesOpaque[0], GL_STATIC_DRAW);

			if (m_IndicesOpaqueCount != 0)
			{
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_Opaque);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER,
							 m_IndicesOpaque.size() * sizeof(uint16_t),
							 &m_IndicesOpaque[0],
							 GL_STATIC_DRAW);
			}

			// vertex attrib
			glEnableVertexAttribArray(0); // Position
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, x));

			glEnableVertexAttribArray(1); // Texture
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, texX));

			glEnableVertexAttribArray(2); // Facing direction
			glVertexAttribPointer(2, 1, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, facing));

			glEnableVertexAttribArray(3); // Occlusion
			glVertexAttribPointer(3, 1, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*) offsetof(Vertex, occlusion));

			glEnableVertexAttribArray(4); // Tint color
			glVertexAttribPointer(4, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*) offsetof(Vertex, tintR));
		}

		m_IndicesCutoutCount = static_cast<unsigned int>(m_IndicesCutout.size());
		if (m_VerticesCutout.size() != 0)
		{

			// bind VAO
			glBindVertexArray(VAO_Cutout);

			// bind VBO & fill
			glBindBuffer(GL_ARRAY_BUFFER, VBO_Cutout);
			glBufferData(
				GL_ARRAY_BUFFER, m_VerticesCutout.size() * sizeof(Vertex), &m_VerticesCutout[0], GL_STATIC_DRAW);

			if (m_IndicesCutoutCount > 0)
			{
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_Cutout);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER,
							 m_IndicesCutout.size() * sizeof(uint16_t),
							 &m_IndicesCutout[0],
							 GL_STATIC_DRAW);
			}

			// vertex attrib
			glEnableVertexAttribArray(0); // Position
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, x));

			glEnableVertexAttribArray(1); // Texture
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, texX));

			glEnableVertexAttribArray(2); // Facing direction
			glVertexAttribPointer(2, 1, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, facing));

			glEnableVertexAttribArray(3); // Occlusion
			glVertexAttribPointer(3, 1, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*) offsetof(Vertex, occlusion));

			glEnableVertexAttribArray(4); // Tint color
			glVertexAttribPointer(4, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*) offsetof(Vertex, tintR));
		}

		m_IndicesTransparentCount = static_cast<unsigned int>(m_IndicesTransparent.size());
		if (m_VerticesTransparent.size() != 0)
		{

			// bind VAO
			glBindVertexArray(VAO_Transparent);

			// bind VBO & fill
			glBindBuffer(GL_ARRAY_BUFFER, VBO_Transparent);
			glBufferData(GL_ARRAY_BUFFER,
						 m_VerticesTransparent.size() * sizeof(Vertex),
						 &m_VerticesTransparent[0],
						 GL_STATIC_DRAW);

			if (m_IndicesTransparentCount > 0)
			{
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_Transparent);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER,
							 m_IndicesTransparent.size() * sizeof(uint16_t),
							 &m_IndicesTransparent[0],
							 GL_STATIC_DRAW);
			}

			// vertex attrib
			glEnableVertexAttribArray(0); // Position
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, x));

			glEnableVertexAttribArray(1); // Texture
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, texX));

			glEnableVertexAttribArray(2); // Facing direction
			glVertexAttribPointer(2, 1, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, facing));

			glEnableVertexAttribArray(3); // Occlusion
			glVertexAttribPointer(3, 1, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*) offsetof(Vertex, occlusion));

			glEnableVertexAttribArray(4); // Tint color
			glVertexAttribPointer(4, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*) offsetof(Vertex, tintR));
		}

		// unbind
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		// Mark buffers as up to date
		m_AreBuffersDataUpToDate = true;

		// Update the vertex count
		m_VertexCount = static_cast<uint32_t>(m_VerticesOpaque.size()) +
			static_cast<uint32_t>(m_VerticesCutout.size()) + static_cast<uint32_t>(m_VerticesTransparent.size());

		// Clear the buffers after setting data to free memory
		std::vector<Vertex>().swap(m_VerticesOpaque);
		std::vector<uint16_t>().swap(m_IndicesOpaque);

		std::vector<Vertex>().swap(m_VerticesCutout);
		std::vector<uint16_t>().swap(m_IndicesCutout);

		std::vector<Vertex>().swap(m_VerticesTransparent);
		std::vector<uint16_t>().swap(m_IndicesTransparent);
	}

	void SubChunkMesh::CleanupOpenGlBuffers()
	{
		glDeleteVertexArrays(1, &VAO_Opaque);
		glDeleteBuffers(1, &VBO_Opaque);
		glDeleteBuffers(1, &EBO_Opaque);

		glDeleteVertexArrays(1, &VAO_Cutout);
		glDeleteBuffers(1, &VBO_Cutout);
		glDeleteBuffers(1, &EBO_Cutout);

		glDeleteVertexArrays(1, &VAO_Transparent);
		glDeleteBuffers(1, &VBO_Transparent);
		glDeleteBuffers(1, &EBO_Transparent);

		VAO_Opaque = 0;
		VBO_Opaque = 0;
		EBO_Opaque = 0;

		VAO_Cutout = 0;
		VBO_Cutout = 0;
		EBO_Cutout = 0;

		VAO_Transparent = 0;
		VBO_Transparent = 0;
		EBO_Transparent = 0;

		m_IndicesOpaqueCount = 0;
		m_IndicesCutoutCount = 0;
		m_IndicesTransparentCount = 0;

		m_AreBuffersGenerated = false;
	}

	void SubChunkMesh::PrepareForRendering()
	{
		if (!m_NeedsToPrepareRendering)
		{
			return; // Already prepared for rendering
		}

		if (!s_TextureAtlas.HasBeenLoaded())
		{
			std::filesystem::path textureAtlasPath = GetMinecraftTexturesPath() / "block" / "dirt.png";

			s_TextureAtlas.LoadFromFile(textureAtlasPath);

			// Set the Texture Unit to 0 for the atlas
			s_Shader.Use();
			s_Shader.setInt("u_Atlas", 0);
		}

		if (!m_AreBuffersGenerated)
		{
			InitOpenGlBuffers(); // Generate OpenGL buffers if they haven't been generated yet
		}

		if (!m_AreBuffersDataUpToDate)
		{
			UpdateOpenGlBuffers(); // Update OpenGL buffers with the latest vertex and index data if they are not up to date
		}

		m_NeedsToPrepareRendering = false; // No longer needs to prepare rendering
	}
} // namespace onion::voxel
