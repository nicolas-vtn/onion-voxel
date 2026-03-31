#pragma once

#include <glm/glm.hpp>

namespace onion::voxel
{
	struct TableLayout
	{
	  public:
		TableLayout() = default;
		TableLayout(int rows, int columns, const glm::ivec2& tableSize, int horizontalSpacing, int verticalSpacing)
			: Rows(rows), Columns(columns), m_TableSize(tableSize), m_HorizontalSpacing(horizontalSpacing),
			  m_VerticalSpacing(verticalSpacing)
		{
		}

	  public:
		int Rows = 1;
		int Columns = 1;

		/// @brief Get the size of each cell in the table, accounting for spacing (In Pixels)
		glm::ivec2 GetCellSize() const
		{
			int totalHorizontalSpacing = (Columns - 1) * m_HorizontalSpacing;
			int totalVerticalSpacing = (Rows - 1) * m_VerticalSpacing;

			return glm::ivec2((m_TableSize.x - totalHorizontalSpacing) / Columns,
							  (m_TableSize.y - totalVerticalSpacing) / Rows);
		}

		glm::vec2 GetCellSize_Float() const
		{
			int totalHorizontalSpacing = (Columns - 1) * m_HorizontalSpacing;
			int totalVerticalSpacing = (Rows - 1) * m_VerticalSpacing;
			return glm::vec2((m_TableSize.x - totalHorizontalSpacing) / (float) Columns,
							 (m_TableSize.y - totalVerticalSpacing) / (float) Rows);
		}

		/// @brief Get the Center Position of the cell at the given row and column index (0-based)
		/// @param row Row index (0-based)
		/// @param column Column index (0-based)
		glm::ivec2 GetElementPosition(int row, int column) const
		{
			glm::vec2 cellSize = GetCellSize_Float();

			float x = column * (cellSize.x + m_HorizontalSpacing) + cellSize.x * 0.5f;
			float y = row * (cellSize.y + m_VerticalSpacing) + cellSize.y * 0.5f;

			return glm::ivec2((int) std::lround(x), (int) std::lround(y));
		}

	  private:
		glm::ivec2 m_TableSize;
		int m_VerticalSpacing = 5;
		int m_HorizontalSpacing = 5;
	};

	class LayoutHelper
	{
	  public:
		static TableLayout CreateTableLayout(
			int rows, int columns, const glm::ivec2& tableSize, int horizontalSpacing, int verticalSpacing)
		{
			TableLayout layout(rows, columns, tableSize, horizontalSpacing, verticalSpacing);
			return layout;
		}
	};
} // namespace onion::voxel
