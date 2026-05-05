#include "UiBlockMesh.hpp"

#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

#include <renderer/gui/colored_background/ColoredBackground.hpp>

namespace onion::voxel
{
	UiBlockMesh::UiBlockMesh(const Inventory& inventory) : m_Inventory(inventory) {}

	UiBlockMesh::~UiBlockMesh()
	{
		if (VBO_Cutout != 0 || VAO_Cutout != 0 || EBO_Cutout != 0 || VBO_Opaque != 0 || VAO_Opaque != 0 ||
			EBO_Opaque != 0 || VBO_Transparent != 0 || VAO_Transparent != 0 || EBO_Transparent != 0)
		{
			std::cerr
				<< "Warning: UiBlockMesh destructor called but OpenGL buffers were not cleaned up. There is a memory leak."
				<< std::endl;
		}
	}

	void UiBlockMesh::Render(const glm::vec2& topLeftPosition, int screenWidth, int screenHeight)
	{
		glm::mat4 viewProjMatrix =
			glm::ortho(0.f, static_cast<float>(screenWidth), static_cast<float>(screenHeight), 0.f, -100.f, 100.f);

		glm::vec2 roundedPosition = glm::round(topLeftPosition);

		s_Shader.Use();
		s_Shader.setMat4("u_ModelMatrix", glm::mat4(1.0f));
		s_Shader.setVec2("u_PositionOffset", roundedPosition);
		s_Shader.setMat4("u_ViewProjMatrix", viewProjMatrix);
		s_Shader.setVec3("u_LightColor", GetLightColor());
		s_Shader.setBool("u_UseFaceShading", true);

		if (m_TextureAtlas)
			m_TextureAtlas->Bind();

		PrepareForRendering();

		// Opaque pass
		PrepareForRenderingOpaque();
		RenderOpaque();

		// Cutout pass — two sub-passes
		PrepareForRenderingCutout();

		// Sub-pass 1: back faces — cull front, no polygon offset
		glCullFace(GL_FRONT);
		glDisable(GL_POLYGON_OFFSET_FILL);
		RenderCutout();

		// Sub-pass 2: front faces — cull back, small negative offset so front wins
		glCullFace(GL_BACK);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(-1.0f, -1.0f);
		RenderCutout();

		// Transparent pass
		PrepareForRenderingTransparent();
		RenderTransparent();

		ResetOpenGLState();

		// Render selected slot highlight
		if (m_RenderSelectedHighlight && m_Inventory.SelectedIndex() >= 0)
		{
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

		// Render stack count labels (Count > 1 only)
		const int rows = m_Inventory.Rows();
		const int cols = m_Inventory.Columns();
		const float textHeight = m_CountLabelTextHeight;
		m_CountLabel.SetTextAlignment(Font::eTextAlignment::Right);
		m_CountLabel.SetTextHeight(textHeight);
		m_CountLabel.SetZOffset(0.75f);

		for (int row = 0; row < rows; ++row)
		{
			for (int col = 0; col < cols; ++col)
			{
				const Slot& slot = m_Inventory.At(row, col);
				if (slot.IsEmpty() || slot.Count <= 1)
					continue;

				const float slotLeft = roundedPosition.x + col * (m_SlotSize.x + m_SlotPadding.x);
				const float slotTop = roundedPosition.y + row * (m_SlotSize.y + m_SlotPadding.y);
				const glm::vec2 labelPos = {slotLeft + m_SlotSize.x - m_SlotBorder,
											slotTop + m_SlotSize.y - textHeight * 0.5f};

				m_CountLabel.SetPosition(labelPos);
				m_CountLabel.SetText(std::to_string(slot.Count));
				m_CountLabel.Render();
			}
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

	void UiBlockMesh::SetCountLabelTextHeight(float height)
	{
		m_CountLabelTextHeight = height;
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

		float withinCellX = localPos.x - col * cellW;
		float withinCellY = localPos.y - row * cellH;
		if (withinCellX >= m_SlotSize.x || withinCellY >= m_SlotSize.y)
			return -1;

		return row * m_Inventory.Columns() + col;
	}

} // namespace onion::voxel
