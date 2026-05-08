#pragma once

#include <cstdint>
#include <utility>
#include <vector>

#include <shared/world/block/BlockId.hpp>

namespace onion::voxel
{
	struct Slot
	{
		BlockId Id = BlockId::Air;
		uint8_t Count = 0;

		bool IsEmpty() const { return Id == BlockId::Air || Count == 0; }

		friend bool operator==(const Slot&, const Slot&) = default;
	};

	static constexpr uint8_t k_MaxStackSize = 64;

	class Inventory
	{
		// ----- Constructor / Destructor -----
	  public:
		Inventory(int rowsCount, int columnsCount) : m_Rows(rowsCount), m_Columns(columnsCount)
		{
			m_Data.resize(rowsCount * columnsCount);
		}

		~Inventory() = default;

		// ----- Public API -----
	  public:
		Slot& At(int row, int column) { return m_Data[row * m_Columns + column]; }
		const Slot At(int row, int column) const { return m_Data[row * m_Columns + column]; }
		Slot& At(int index) { return m_Data[index]; }
		const Slot At(int index) const { return m_Data[index]; }

		int& SelectedIndex() { return m_SelectedIndex; }
		const int& SelectedIndex() const { return m_SelectedIndex; }

		std::pair<int, int> GetRowColumnFromIndex(int index) const
		{
			int row = index / m_Columns;
			int column = index % m_Columns;
			return {row, column};
		}

		std::vector<Slot>& Content() { return m_Data; }
		const std::vector<Slot>& Content() const { return m_Data; }

		int Rows() const { return m_Rows; }
		int Columns() const { return m_Columns; }

		// ----- Private Members -----
	  private:
		int m_Rows;
		int m_Columns;
		int m_SelectedIndex = -1;
		std::vector<Slot> m_Data;

		// ----- Operators -----
	  public:
		friend bool operator==(const Inventory& lhs, const Inventory& rhs)
		{
			return lhs.Rows() == rhs.Rows() && lhs.Columns() == rhs.Columns() && lhs.Content() == rhs.Content();
		}

		friend bool operator!=(const Inventory& lhs, const Inventory& rhs) { return !(lhs == rhs); }
	};
} // namespace onion::voxel
