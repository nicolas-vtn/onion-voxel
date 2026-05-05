#pragma once

#include <glm/glm.hpp>

#include <atomic>

#include <renderer/gui/controls/label/Label.hpp>
#include <renderer/item_mesh/ItemMesh.hpp>

#include <shared/entities/components/Inventory.hpp>

namespace onion::voxel
{
	class MeshBuilder;

	class UiBlockMesh : public ItemMesh
	{
		friend class MeshBuilder;

	  public:
		// ----- Constructor / Destructor -----
		UiBlockMesh(const Inventory& inventory);
		~UiBlockMesh();

		// ----- Public API -----
	  public:
		void Render(const glm::vec2& topLeftPosition, int screenWidth, int screenHeight);

		void SetInventory(const Inventory& inventory, const glm::vec2& slotSize, const glm::vec2& slotPadding);

		/// @brief Sets the inner border in pixels. The block is scaled to fit slotSize - 2*border,
		/// keeping it visually smaller than the full slot area. Marks the mesh dirty if changed.
		void SetSlotBorder(float border);
		float GetSlotBorder() const;

		/// @brief Sets the text height for the per-slot stack count label. Pass GuiElement::s_TextHeight.
		void SetCountLabelTextHeight(float height);

		bool IsDirty() const;
		void SetDirty(bool isDirty);

		bool GetRenderSelectedHighlight() const;
		void SetRenderSelectedHighlight(bool renderSelectedHighlight);

		int GetSelectedIndexFromCursorPosition(const glm::vec2& cursorPosition,
											   const glm::vec2& topLeftPosition) const;

		// ----- Members -----
	  private:
		Inventory m_Inventory;
		glm::vec2 m_SlotSize{50.f, 50.f};	  // In pixels
		glm::vec2 m_SlotPadding{10.f, 10.f}; // In pixels
		float m_SlotBorder{4.f};			  // Inner border in pixels

		Label m_CountLabel{"UiBlockMesh_CountLabel"};
		float m_CountLabelTextHeight{16.f};

		// ----- States -----
	  private:
		std::atomic_bool m_IsDirty{true};
		std::atomic_bool m_RenderSelectedHighlight{true};
	};

} // namespace onion::voxel
