#include "ItemMesh.hpp"

#include <iostream>

#include <renderer/assets_manager/AssetsManager.hpp>

namespace onion::voxel
{
	// ----- Static Initialization -----
	Shader ItemMesh::s_Shader{
		AssetsManager::GetShadersDirectory() / "blocks_ui.vert",
		AssetsManager::GetShadersDirectory() / "blocks_ui.frag",
	};

	// ----- Public API -----

	void ItemMesh::Delete()
	{
		CleanupOpenGlBuffers();
	}

	uint32_t ItemMesh::GetVertexCount() const
	{
		return m_VertexCount;
	}

	// ----- Static Settings -----

	void ItemMesh::SetLightColor(const glm::vec3& lightColor)
	{
		s_LightColor = lightColor;
		s_Shader.Use();
		s_Shader.setVec3("u_LightColor", lightColor);
	}

	glm::vec3 ItemMesh::GetLightColor()
	{
		return s_LightColor;
	}

	void ItemMesh::SetUseFaceShading(bool useFaceShading)
	{
		s_UseFaceShading = useFaceShading;
		s_Shader.Use();
		s_Shader.setBool("u_UseFaceShading", useFaceShading);
	}

	bool ItemMesh::GetUseFaceShading()
	{
		return s_UseFaceShading;
	}

	void ItemMesh::SetUseOcclusion(bool useOcclusion)
	{
		s_UseOcclusion = useOcclusion;
		s_Shader.Use();
		s_Shader.setBool("u_UseOcclusion", useOcclusion);
	}

	bool ItemMesh::GetUseOcclusion()
	{
		return s_UseOcclusion;
	}

	// ----- Buffer Lifecycle -----

	void ItemMesh::BuffersUpdated()
	{
		m_AreBuffersDataUpToDate = false;
		m_NeedsToPrepareRendering = true;
	}

	void ItemMesh::PrepareForRendering()
	{
		if (!m_NeedsToPrepareRendering)
			return;

		if (!m_AreBuffersGenerated)
			InitOpenGlBuffers();

		if (!m_AreBuffersDataUpToDate)
			UpdateOpenGlBuffers();

		m_NeedsToPrepareRendering = false;
	}

	// ----- Render Pass Helpers -----

	void ItemMesh::PrepareForRenderingOpaque()
	{
		s_Shader.Use();
		s_Shader.setBool("u_RenderCutout", false);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	void ItemMesh::RenderOpaque()
	{
		if (m_NeedsToPrepareRendering)
			PrepareForRendering();

		if (m_IndicesOpaqueCount > 0)
		{
			glBindVertexArray(VAO_Opaque);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_Opaque);
			glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_IndicesOpaqueCount), GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
		}
	}

	void ItemMesh::PrepareForRenderingCutout()
	{
		s_Shader.Use();
		s_Shader.setBool("u_RenderCutout", true);

		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	void ItemMesh::RenderCutout()
	{
		if (m_IndicesCutoutCount > 0)
		{
			glBindVertexArray(VAO_Cutout);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_Cutout);
			glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_IndicesCutoutCount), GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
		}
	}

	void ItemMesh::PrepareForRenderingTransparent()
	{
		s_Shader.Use();
		s_Shader.setBool("u_RenderCutout", false);

		glDisable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(-1.0f, -1.0f);
	}

	void ItemMesh::RenderTransparent()
	{
		if (m_IndicesTransparentCount > 0)
		{
			glBindVertexArray(VAO_Transparent);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_Transparent);
			glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_IndicesTransparentCount), GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
		}
	}

	void ItemMesh::ResetOpenGLState()
	{
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
		glDisable(GL_POLYGON_OFFSET_FILL);
		glDisable(GL_CULL_FACE);
	}

	// ----- OpenGL Buffer Management -----

	void ItemMesh::InitOpenGlBuffers()
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

	void ItemMesh::UpdateOpenGlBuffers()
	{
		std::unique_lock lock(m_Mutex);

		auto uploadPass = [&](unsigned int vao,
							  unsigned int vbo,
							  unsigned int ebo,
							  std::vector<Vertex>& verts,
							  std::vector<uint32_t>& idxs,
							  unsigned int& idxCount)
		{
			idxCount = static_cast<unsigned int>(idxs.size());

			if (verts.empty())
				return;

			glBindVertexArray(vao);

			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);

			if (idxCount > 0)
			{
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, idxs.size() * sizeof(uint32_t), idxs.data(), GL_STATIC_DRAW);
			}

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, x));

			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, texX));

			glEnableVertexAttribArray(2);
			glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*) offsetof(Vertex, facing));

			glEnableVertexAttribArray(3);
			glVertexAttribPointer(
				3, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*) offsetof(Vertex, tintR));
		};

		uploadPass(VAO_Opaque, VBO_Opaque, EBO_Opaque, m_VerticesOpaque, m_IndicesOpaque, m_IndicesOpaqueCount);
		uploadPass(
			VAO_Cutout, VBO_Cutout, EBO_Cutout, m_VerticesCutout, m_IndicesCutout, m_IndicesCutoutCount);
		uploadPass(VAO_Transparent,
				   VBO_Transparent,
				   EBO_Transparent,
				   m_VerticesTransparent,
				   m_IndicesTransparent,
				   m_IndicesTransparentCount);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		m_AreBuffersDataUpToDate = true;

		m_VertexCount = static_cast<uint32_t>(m_VerticesOpaque.size()) +
			static_cast<uint32_t>(m_VerticesCutout.size()) +
			static_cast<uint32_t>(m_VerticesTransparent.size());

		// Free CPU memory after upload
		std::vector<Vertex>().swap(m_VerticesOpaque);
		std::vector<uint32_t>().swap(m_IndicesOpaque);
		std::vector<Vertex>().swap(m_VerticesCutout);
		std::vector<uint32_t>().swap(m_IndicesCutout);
		std::vector<Vertex>().swap(m_VerticesTransparent);
		std::vector<uint32_t>().swap(m_IndicesTransparent);
	}

	void ItemMesh::CleanupOpenGlBuffers()
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

		VAO_Opaque = VBO_Opaque = EBO_Opaque = 0;
		VAO_Cutout = VBO_Cutout = EBO_Cutout = 0;
		VAO_Transparent = VBO_Transparent = EBO_Transparent = 0;

		m_IndicesOpaqueCount = 0;
		m_IndicesCutoutCount = 0;
		m_IndicesTransparentCount = 0;

		m_AreBuffersGenerated = false;
	}

} // namespace onion::voxel
