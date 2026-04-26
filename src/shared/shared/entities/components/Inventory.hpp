#pragma once

#include <vector>

#include <shared/world/block/BlockId.hpp>

namespace onion::voxel
{
	class Inventory
	{
	  public:
		Inventory(int rowsCount, int columnsCount) : m_Rows(rowsCount), m_Columns(columnsCount)
		{
			m_Data.resize(rowsCount * columnsCount, BlockId::Air);
		}

		BlockId& At(int row, int column) { return m_Data[row * m_Columns + column]; }
		BlockId& At(int index) { return m_Data[index]; }

		int& SelectedIndex() { return m_SelectedIndex; }
		const int& SelectedIndex() const { return m_SelectedIndex; }

		std::vector<BlockId>& Content() { return m_Data; }
		const std::vector<BlockId>& Content() const { return m_Data; }

		int Rows() const { return m_Rows; }
		int Columns() const { return m_Columns; }

	  private:
		int m_Rows;
		int m_Columns;
		int m_SelectedIndex = -1;
		std::vector<BlockId> m_Data;
	};
} // namespace onion::voxel
