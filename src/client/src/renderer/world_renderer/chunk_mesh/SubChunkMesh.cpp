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

	SubChunkMesh::~SubChunkMesh() {}

	void SubChunkMesh::RenderOpaque()
	{
		if (m_NeedsToPrepareRendering)
		{
			PrepareForRendering();
		}

		glDepthMask(GL_TRUE); // Write to depth buffer
		glDisable(GL_BLEND);

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
		if (m_NeedsToPrepareRendering)
		{
			PrepareForRendering();
		}

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		glDisable(GL_BLEND); // << no blending for cutout
		// glDepthMask(GL_FALSE); // overlays don’t write depth (optional, see note)

		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(-1.0f, -1.0f); // pull slightly towards camera for coplanar overlays

		glBindVertexArray(VAO_Cutout);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_Cutout);
		glDrawElements(GL_TRIANGLES, (GLsizei) m_IndicesCutoutCount, GL_UNSIGNED_SHORT, 0);
		glBindVertexArray(0);

		glDisable(GL_POLYGON_OFFSET_FILL);
		glDepthMask(GL_TRUE); // restore
	}

	void SubChunkMesh::RenderTransparent()
	{
		if (m_NeedsToPrepareRendering)
		{
			PrepareForRendering();
		}

		// glDepthMask(GL_FALSE); // Don't write to depth buffer
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		if (m_IndicesTransparentCount > 0)
		{
			glEnable(GL_POLYGON_OFFSET_FILL);
			glPolygonOffset(-1.0f, -1.0f); // negative values pull overlays forward a bit
			glBindVertexArray(VAO_Transparent);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_Transparent);
			glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_IndicesTransparentCount), GL_UNSIGNED_SHORT, 0);
			glBindVertexArray(0);
			glDisable(GL_POLYGON_OFFSET_FILL); // (Optional, disables after overlays)
		}

		// glDepthMask(GL_TRUE); // Reset state
		glDisable(GL_BLEND);
	}

	bool SubChunkMesh::IsDirty() const
	{
		return m_IsDirty;
	}

	void SubChunkMesh::SetDirty(bool isDirty)
	{
		m_IsDirty = isDirty;
	}

	bool SubChunkMesh::IsGenerating() const
	{
		return m_IsGenerating;
	}

	void SubChunkMesh::SetGenerating(bool isGenerating)
	{
		m_IsGenerating = isGenerating;
	}

	bool SubChunkMesh::IsEmpty() const
	{
		return m_IsEmpty;
	}
	void SubChunkMesh::InitOpenGlBuffers()
	{
		CleanupOpenGlBuffers();

		glGenVertexArrays(1, &VAO_Opaque);
		glGenBuffers(1, &VBO_Opaque);
		glGenBuffers(1, &EBO_Opaque);
		glGenBuffers(1, &EBO_OpaqueOverlay);

		glGenVertexArrays(1, &VAO_Cutout);
		glGenBuffers(1, &VBO_Cutout);
		glGenBuffers(1, &EBO_Cutout);
		glGenBuffers(1, &EBO_Cutout);

		glGenVertexArrays(1, &VAO_Transparent);
		glGenBuffers(1, &VBO_Transparent);
		glGenBuffers(1, &EBO_Transparent);
		glGenBuffers(1, &EBO_TransparentOverlay);

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
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VERTEX_SIZE * sizeof(float), (void*) 0);
			glEnableVertexAttribArray(0); // Position

			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, VERTEX_SIZE * sizeof(float), (void*) (3 * sizeof(float)));
			glEnableVertexAttribArray(1); // Texture

			glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, VERTEX_SIZE * sizeof(float), (void*) (5 * sizeof(float)));
			glEnableVertexAttribArray(2); // Facing direction

			glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, VERTEX_SIZE * sizeof(float), (void*) (6 * sizeof(float)));
			glEnableVertexAttribArray(3); // Occlusion

			glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, VERTEX_SIZE * sizeof(float), (void*) (7 * sizeof(float)));
			glEnableVertexAttribArray(4); // Tint color

			glEnableVertexAttribArray(0); // Position
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
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VERTEX_SIZE * sizeof(float), (void*) 0);
			glEnableVertexAttribArray(0); // Position

			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, VERTEX_SIZE * sizeof(float), (void*) (3 * sizeof(float)));
			glEnableVertexAttribArray(1); // Texture

			glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, VERTEX_SIZE * sizeof(float), (void*) (5 * sizeof(float)));
			glEnableVertexAttribArray(2); // Facing direction

			glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, VERTEX_SIZE * sizeof(float), (void*) (6 * sizeof(float)));
			glEnableVertexAttribArray(3); // Occlusion

			glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, VERTEX_SIZE * sizeof(float), (void*) (7 * sizeof(float)));
			glEnableVertexAttribArray(4); // Tint color
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
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VERTEX_SIZE * sizeof(float), (void*) 0);
			glEnableVertexAttribArray(0); // Position

			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, VERTEX_SIZE * sizeof(float), (void*) (3 * sizeof(float)));
			glEnableVertexAttribArray(1); // Texture

			glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, VERTEX_SIZE * sizeof(float), (void*) (5 * sizeof(float)));
			glEnableVertexAttribArray(2); // Facing direction

			glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, VERTEX_SIZE * sizeof(float), (void*) (6 * sizeof(float)));
			glEnableVertexAttribArray(3); // Occlusion

			glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, VERTEX_SIZE * sizeof(float), (void*) (7 * sizeof(float)));
			glEnableVertexAttribArray(4); // Tint color
		}

		// unbind
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		m_AreBuffersDataSet = true;

		// Clear the buffers after setting data to free memory
		std::vector<Vertex>().swap(m_VerticesOpaque);
		std::vector<uint16_t>().swap(m_IndicesOpaque);

		std::vector<Vertex>().swap(m_VerticesTransparent);
		std::vector<uint16_t>().swap(m_IndicesTransparent);
	}

	void SubChunkMesh::CleanupOpenGlBuffers()
	{
		glDeleteVertexArrays(1, &VAO_Opaque);
		glDeleteBuffers(1, &VBO_Opaque);
		glDeleteBuffers(1, &EBO_Opaque);
		glDeleteBuffers(1, &EBO_OpaqueOverlay);

		glDeleteVertexArrays(1, &VAO_Cutout);
		glDeleteBuffers(1, &VBO_Cutout);
		glDeleteBuffers(1, &EBO_Cutout);
		glDeleteBuffers(1, &EBO_Cutout);

		glDeleteVertexArrays(1, &VAO_Transparent);
		glDeleteBuffers(1, &VBO_Transparent);
		glDeleteBuffers(1, &EBO_Transparent);
		glDeleteBuffers(1, &EBO_TransparentOverlay);

		VAO_Opaque = 0;
		VBO_Opaque = 0;
		EBO_Opaque = 0;
		EBO_OpaqueOverlay = 0;

		VAO_Transparent = 0;
		VBO_Transparent = 0;
		EBO_Transparent = 0;
		EBO_TransparentOverlay = 0;

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

		if (!m_AreBuffersDataSet)
		{
			UpdateOpenGlBuffers(); // Set data to OpenGL buffers if they haven't been set yet
		}

		m_IsReadyToRender = true;		   // Mark as ready to render
		m_NeedsToPrepareRendering = false; // No longer needs to prepare rendering
	}
} // namespace onion::voxel
