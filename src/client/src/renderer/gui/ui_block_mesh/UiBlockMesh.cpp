#include "UiBlockMesh.hpp"

#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

#include <renderer/assets_manager/AssetsManager.hpp>

#include <renderer/gui/colored_background/ColoredBackground.hpp>

namespace onion::voxel
{
	// ----- Static Initialization -----
	Shader UiBlockMesh::s_Shader{
		AssetsManager::GetShadersDirectory() / "blocks_ui.vert",
		AssetsManager::GetShadersDirectory() / "blocks_ui.frag",
	};

	UiBlockMesh::UiBlockMesh(const Inventory& inventory) : m_Inventory(inventory) {}

	UiBlockMesh::~UiBlockMesh()
	{
		if (VBO_Cutout != 0 || VAO_Cutout != 0 || EBO_Cutout != 0 || VBO_Opaque != 0 || VAO_Opaque != 0 ||
			EBO_Opaque != 0 || VBO_Transparent != 0 || VAO_Transparent != 0 || EBO_Transparent != 0)
		{
			std::cerr << "Warning: UiBlockMesh destructor called but OpenGL buffers were not cleaned up. There is a "
						 "memory leak."
					  << std::endl;
		}
	}

	void UiBlockMesh::PrepareForRenderingOpaque()
	{
		// ----- Shader Setup -----
		s_Shader.Use();
		s_Shader.setBool("u_RenderCutout", false);

		// ----- OpenGL State Setup -----
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	void UiBlockMesh::RenderOpaque()
	{
		if (m_NeedsToPrepareRendering)
		{
			PrepareForRendering();
		}

		if (m_IndicesOpaqueCount > 0)
		{
			glBindVertexArray(VAO_Opaque);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_Opaque);
			glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_IndicesOpaqueCount), GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
		}
	}

	void UiBlockMesh::PrepareForRenderingCutout()
	{ // ----- Shader Setup -----
		s_Shader.Use();
		s_Shader.setBool("u_RenderCutout", true);

		// ----- OpenGL State Setup -----
		// Face culling is enabled to avoid back-face / front-face z-fighting on coplanar
		// double-sided geometry (e.g. leaf quads). The cutout pass is split into two
		// sub-passes in Render(): back faces first (no offset), then front faces with a
		// small negative polygon offset so front faces always win over back faces.
		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
		// Polygon offset is managed per sub-pass in Render(); disable it here as the
		// back-face sub-pass does not need it.
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	void UiBlockMesh::RenderCutout()
	{
		if (m_IndicesCutoutCount > 0)
		{
			glBindVertexArray(VAO_Cutout);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_Cutout);
			glDrawElements(GL_TRIANGLES, (GLsizei) m_IndicesCutoutCount, GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
		}
	}

	void UiBlockMesh::PrepareForRenderingTransparent()
	{
		// ----- Shader Setup -----
		s_Shader.Use();
		s_Shader.setBool("u_RenderCutout", false);

		// ----- OpenGL State Setup -----
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

	void UiBlockMesh::RenderTransparent()
	{
		if (m_IndicesTransparentCount > 0)
		{
			glBindVertexArray(VAO_Transparent);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_Transparent);
			glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_IndicesTransparentCount), GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
		}
	}

	void UiBlockMesh::ResetOpenGLState()
	{
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	void UiBlockMesh::Render(const glm::vec2& topLeftPosition, int screenWidth, int screenHeight)
	{
		glm::mat4 viewProjMatrix =
			glm::ortho(0.f, static_cast<float>(screenWidth), static_cast<float>(screenHeight), 0.f, -100.f, 100.f);

		// Render Debug Panel
		//if (EngineContext::Get().ShowDebugMenus)
		//	RenderDebugPanel();

		glm::vec2 roundedPosition = glm::round(topLeftPosition);

		s_Shader.Use();
		s_Shader.setVec2("u_PositionOffset", roundedPosition);
		s_Shader.setMat4("u_ViewProjMatrix", viewProjMatrix);
		s_Shader.setVec3("u_LightColor", s_LightColor);
		s_Shader.setBool("u_UseFaceShading", true);

		if (m_TextureAtlas)
			m_TextureAtlas->Bind();

		PrepareForRendering();

		// Render Opaque Block Mesh
		PrepareForRenderingOpaque();
		RenderOpaque();

		// Render Cutout BlockMesh
		PrepareForRenderingCutout();

		// Sub-pass 1: back faces — cull front faces, no polygon offset.
		// Back faces are rendered first and write to the depth buffer.
		glCullFace(GL_FRONT);
		glDisable(GL_POLYGON_OFFSET_FILL);

		RenderCutout();

		// Sub-pass 2: front faces — cull back faces, small negative polygon offset
		// so front faces are shifted slightly toward the camera and always win over
		// the back faces written in sub-pass 1.
		glCullFace(GL_BACK);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(-1.0f, -1.0f);
		RenderCutout();

		// Render Transparent Block Mesh
		PrepareForRenderingTransparent();
		RenderTransparent();

		ResetOpenGLState();

		// Render Selected Highlight
		if (m_RenderSelectedHighlight && m_Inventory.SelectedIndex() >= 0)
		{
			// Retreves coordinates of the selected slot
			const int& selectedIndex = m_Inventory.SelectedIndex();
			const auto& [selectedRow, selectedColumn] = m_Inventory.GetRowColumnFromIndex(selectedIndex);
			glm::vec2 selectedSlotPos = roundedPosition +
				glm::vec2(selectedColumn * (m_SlotSize.x + m_SlotPadding.x),
						  selectedRow * (m_SlotSize.y + m_SlotPadding.y));
			glm::vec2 selectedSlotBottomRight = selectedSlotPos + m_SlotSize;

			constexpr glm::vec4 highlightColor{1.f, 1.f, 1.f, 0.5f};
			ColoredBackground::CornerOptions highlightOptions;
			highlightOptions.TopLeftCorner = glm::ivec2(selectedSlotPos + glm::vec2(m_SlotBorder));
			highlightOptions.BottomRightCorner = glm::ivec2(selectedSlotBottomRight - glm::vec2(m_SlotBorder));
			highlightOptions.Color = highlightColor;
			highlightOptions.ZOffset = 0.7f;
			ColoredBackground::Render(highlightOptions);
		}
	}

	void UiBlockMesh::SetInventory(const Inventory& inventory, const glm::vec2& slotSize, const glm::vec2& slotPadding)
	{
		if (inventory != m_Inventory || slotSize != m_SlotSize || slotPadding != m_SlotPadding)
		{
			m_Inventory = inventory;
			m_SlotSize = slotSize;
			m_SlotPadding = slotPadding;
			SetDirty(true);
		}

		// Override inventory selected index anyway
		m_Inventory.SelectedIndex() = inventory.SelectedIndex();
	}

	void UiBlockMesh::SetSlotBorder(float border)
	{
		if (border == m_SlotBorder)
			return;
		m_SlotBorder = border;
		SetDirty(true);
	}

	float UiBlockMesh::GetSlotBorder() const
	{
		return m_SlotBorder;
	}

	void UiBlockMesh::Delete()
	{
		CleanupOpenGlBuffers();
	}

	uint32_t UiBlockMesh::GetVertexCount() const
	{
		return m_VertexCount;
	}

	bool UiBlockMesh::IsDirty() const
	{
		return m_IsDirty;
	}

	void UiBlockMesh::SetDirty(bool isDirty)
	{
		m_IsDirty = isDirty;
	}

	bool UiBlockMesh::GetRenderSelectedHighlight() const
	{
		return m_RenderSelectedHighlight;
	}

	void UiBlockMesh::SetRenderSelectedHighlight(bool renderSelectedHighlight)
	{
		m_RenderSelectedHighlight = renderSelectedHighlight;
	}

	int UiBlockMesh::GetSelectedIndexFromCursorPosition(const glm::vec2& cursorPosition,
														const glm::vec2& topLeftPosition) const
	{
		glm::vec2 localPos = cursorPosition - topLeftPosition;

		if (localPos.x < 0.f || localPos.y < 0.f)
			return -1;

		const float cellW = m_SlotSize.x + m_SlotPadding.x;
		const float cellH = m_SlotSize.y + m_SlotPadding.y;

		int col = (int) (localPos.x / cellW);
		int row = (int) (localPos.y / cellH);

		if (col < 0 || col >= m_Inventory.Columns() || row < 0 || row >= m_Inventory.Rows())
			return -1;

		// Exclude the padding region within the cell
		float withinCellX = localPos.x - col * cellW;
		float withinCellY = localPos.y - row * cellH;
		if (withinCellX >= m_SlotSize.x || withinCellY >= m_SlotSize.y)
			return -1;

		return row * m_Inventory.Columns() + col;
	}

	void UiBlockMesh::SetLightColor(const glm::vec3& lightColor)
	{
		s_LightColor = lightColor;
		s_Shader.Use();
		s_Shader.setVec3("u_LightColor", lightColor);
	}

	glm::vec3 UiBlockMesh::GetLightColor()
	{
		return s_LightColor;
	}

	void UiBlockMesh::SetUseFaceShading(bool useFaceShading)
	{
		s_UseFaceShading = useFaceShading;
		s_Shader.Use();
		s_Shader.setBool("u_UseFaceShading", useFaceShading);
	}

	bool UiBlockMesh::GetUseFaceShading()
	{
		return s_UseFaceShading;
	}

	void UiBlockMesh::SetUseOcclusion(bool useOcclusion)
	{
		s_UseOcclusion = useOcclusion;
		s_Shader.Use();
		s_Shader.setBool("u_UseOcclusion", useOcclusion);
	}

	bool UiBlockMesh::GetUseOcclusion()
	{
		return s_UseOcclusion;
	}

	void UiBlockMesh::BuffersUpdated()
	{
		m_AreBuffersDataUpToDate = false;
		m_NeedsToPrepareRendering = true;
	}

	void UiBlockMesh::InitOpenGlBuffers()
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

	void UiBlockMesh::UpdateOpenGlBuffers()
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
							 m_IndicesOpaque.size() * sizeof(uint32_t),
							 &m_IndicesOpaque[0],
							 GL_STATIC_DRAW);
			}

			// vertex attrib
			glEnableVertexAttribArray(0); // Position x y z
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, x));

			glEnableVertexAttribArray(1); // Texture
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, texX));

			glEnableVertexAttribArray(2); // Facing direction
			glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*) offsetof(Vertex, facing));

			glEnableVertexAttribArray(3); // Tint color
			glVertexAttribPointer(3, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*) offsetof(Vertex, tintR));
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
							 m_IndicesCutout.size() * sizeof(uint32_t),
							 &m_IndicesCutout[0],
							 GL_STATIC_DRAW);
			}

			// vertex attrib
			glEnableVertexAttribArray(0); // Position x y z
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, x));

			glEnableVertexAttribArray(1); // Texture
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, texX));

			glEnableVertexAttribArray(2); // Facing direction
			glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*) offsetof(Vertex, facing));

			glEnableVertexAttribArray(3); // Tint color
			glVertexAttribPointer(3, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*) offsetof(Vertex, tintR));
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
							 m_IndicesTransparent.size() * sizeof(uint32_t),
							 &m_IndicesTransparent[0],
							 GL_STATIC_DRAW);
			}

			// vertex attrib
			glEnableVertexAttribArray(0); // Position x y z
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, x));

			glEnableVertexAttribArray(1); // Texture
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, texX));

			glEnableVertexAttribArray(2); // Facing direction
			glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*) offsetof(Vertex, facing));

			glEnableVertexAttribArray(3); // Tint color
			glVertexAttribPointer(3, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*) offsetof(Vertex, tintR));
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
		std::vector<uint32_t>().swap(m_IndicesOpaque);

		std::vector<Vertex>().swap(m_VerticesCutout);
		std::vector<uint32_t>().swap(m_IndicesCutout);

		std::vector<Vertex>().swap(m_VerticesTransparent);
		std::vector<uint32_t>().swap(m_IndicesTransparent);
	}

	void UiBlockMesh::CleanupOpenGlBuffers()
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

	void UiBlockMesh::PrepareForRendering()
	{
		if (!m_NeedsToPrepareRendering)
		{
			return; // Already prepared for rendering
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
