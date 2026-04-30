#pragma once

#include <renderer/gui/GuiElement.hpp>
#include <renderer/gui/controls/button/Button.hpp>
#include <renderer/gui/controls/label/Label.hpp>
#include <renderer/gui/controls/sprite/Sprite.hpp>
#include <renderer/gui/controls/text_field/TextField.hpp>
#include <renderer/gui/controls/tooltip/Tooltip.hpp>

#include <renderer/gui/ui_block_mesh/UiBlockMesh.hpp>

namespace onion::voxel
{
	class InventoryPanel : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		InventoryPanel(const std::string& name);
		~InventoryPanel() override;

		// ----- Public API -----
	  public:
		void Render() override;
		void Initialize() override;
		void Delete() override;
		void ReloadTextures() override;

		// ----- Public Events -----
	  public:
		Event<const GuiElement*> EvtRequestBackNavigation;

		// ----- Private Members -----
	  private:
		Inventory m_CraftingInventory{2, 2};
		Inventory m_CraftingOutputInventory{1, 1};

		// ----- Controls -----
	  private:
		Sprite m_InventoryBackground_Sprite;
		Label m_Crafting_Label;
		Tooltip m_Tooltip;
		Button m_PreviousPage_Button;
		Button m_NextPage_Button;
		Label m_PageIndicator_Label;
		TextField m_Search_TextField;
		Sprite m_ArmorHelmet_Sprite;
		Sprite m_ArmorChestplate_Sprite;
		Sprite m_ArmorLeggings_Sprite;
		Sprite m_ArmorBoots_Sprite;
		Sprite m_Offhand_Sprite;

		std::shared_ptr<UiBlockMesh> m_HotbarBlockMesh = std::make_shared<UiBlockMesh>(Inventory{1, 9});
		std::shared_ptr<UiBlockMesh> m_InventoryBlockMesh = std::make_shared<UiBlockMesh>(Inventory{3, 9});
		std::shared_ptr<UiBlockMesh> m_CreativeBlockMesh = std::make_shared<UiBlockMesh>(Inventory{10, 7});

		std::shared_ptr<UiBlockMesh> m_ArmorBlockMesh = std::make_shared<UiBlockMesh>(Inventory{4, 1});
		std::shared_ptr<UiBlockMesh> m_OffhandBlockMesh = std::make_shared<UiBlockMesh>(Inventory{1, 1});
		std::shared_ptr<UiBlockMesh> m_CraftingGridBlockMesh = std::make_shared<UiBlockMesh>(Inventory{2, 2});
		std::shared_ptr<UiBlockMesh> m_CraftingOutputBlockMesh = std::make_shared<UiBlockMesh>(Inventory{1, 1});

		// ----- Textures -----
	  private:
		static inline const std::filesystem::path s_PathInventoryBackground =
			GuiElement::s_BasePathGuiAssets / "container" / "inventory.png";

		static inline const std::filesystem::path s_PathSlots =
			GuiElement::s_BasePathGuiAssets / "sprites" / "container" / "slot";

		static inline const std::filesystem::path s_PathArmorHelmet = s_PathSlots / "helmet.png";
		static inline const std::filesystem::path s_PathArmorChestplate = s_PathSlots / "chestplate.png";
		static inline const std::filesystem::path s_PathArmorLeggings = s_PathSlots / "leggings.png";
		static inline const std::filesystem::path s_PathArmorBoots = s_PathSlots / "boots.png";
		static inline const std::filesystem::path s_PathOffhand = s_PathSlots / "shield.png";

		// ----- Private Members -----
	  private:
		static const std::vector<BlockId>& GetCreativeTabBlockIds();
		Inventory m_CreativeTabInventory{10, 7};
		std::vector<BlockId> m_FilteredCreativeTabBlockIds;
		int m_CurrentPageIndex = 0;
		int m_MaxPageIndex = 0;
		std::string m_LastSearchQuery = "hqsdfdjshfgjdhsgfdhsgfds";

		void UpdateFilteredCreativeTabBlockIds(const std::string& search);
		int ComputeMaxPageIndex(int itemCount) const;

		// ----- Internal Event Subscription and Handlers -----
	  private:
		std::vector<EventHandle> m_EventHandles;
		void SubscribeToControlEvents();

		void Handle_PreviousPageButtonClick(const Button& button);
		void Handle_NextPageButtonClick(const Button& button);
	};
} // namespace onion::voxel
