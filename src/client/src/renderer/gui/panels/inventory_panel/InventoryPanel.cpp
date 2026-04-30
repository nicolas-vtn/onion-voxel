#include "InventoryPanel.hpp"

#include <algorithm>

#include <renderer/EngineContext.hpp>
#include <renderer/gui/colored_background/ColoredBackground.hpp>

#include <renderer/world_renderer/WorldRenderer.hpp>

namespace
{
	std::string BuildTooltipText(const onion::voxel::BlockId blockId)
	{
		using namespace onion::voxel;
		const std::string blockName = BlockIds::GetName(blockId);
		const std::string line2 = "§9§oOnion::Voxel§r§r";
		const std::string tooltipText = blockName + "\n" + line2;
		return tooltipText;
	}
} // namespace

namespace onion::voxel
{
	InventoryPanel::InventoryPanel(const std::string& name)
		: GuiElement(name), m_InventoryBackground_Sprite(
								"InventoryBackground_Sprite", s_PathInventoryBackground, Sprite::eOrigin::ResourcePack),
		  m_Crafting_Label("Crafting_Label"), m_Tooltip("InventoryTooltip"),
		  m_PreviousPage_Button("PreviousPage_Button"), m_NextPage_Button("NextPage_Button"),
		  m_PageIndicator_Label("PageIndicator_Label"), m_Search_TextField("Search_TextField"),
		  m_ArmorHelmet_Sprite("ArmorHelmet_Sprite", s_PathArmorHelmet, Sprite::eOrigin::ResourcePack),
		  m_ArmorChestplate_Sprite("ArmorChestplate_Sprite", s_PathArmorChestplate, Sprite::eOrigin::ResourcePack),
		  m_ArmorLeggings_Sprite("ArmorLeggings_Sprite", s_PathArmorLeggings, Sprite::eOrigin::ResourcePack),
		  m_ArmorBoots_Sprite("ArmorBoots_Sprite", s_PathArmorBoots, Sprite::eOrigin::ResourcePack),
		  m_Offhand_Sprite("Offhand_Sprite", s_PathOffhand, Sprite::eOrigin::ResourcePack)
	{
		SubscribeToControlEvents();

		m_InventoryBackground_Sprite.SetZOffset(0.55f); // Ensure it's in front of the Hotbar

		m_ArmorHelmet_Sprite.SetZOffset(0.56f);
		m_ArmorChestplate_Sprite.SetZOffset(0.56f);
		m_ArmorLeggings_Sprite.SetZOffset(0.56f);
		m_ArmorBoots_Sprite.SetZOffset(0.56f);
		m_Offhand_Sprite.SetZOffset(0.56f);

		m_Crafting_Label.SetZOffset(0.6f); // Ensure it's in front of the inventory background
		m_Crafting_Label.SetText("Crafting");
		glm::vec3 craftingTextColor = {63 / 255.f, 63 / 255.f, 63 / 255.f}; // Light gray color
		m_Crafting_Label.SetCustomTextColor(craftingTextColor);
		m_Crafting_Label.EnableShadow(false);
		m_Crafting_Label.SetTextAlignment(Font::eTextAlignment::Left);

		m_Tooltip.SetZOffset(0.85f);

		m_PreviousPage_Button.SetText("◀");

		m_NextPage_Button.SetText("▶");

		m_PageIndicator_Label.SetTextAlignment(Font::eTextAlignment::Center);
		m_PageIndicator_Label.SetZOffset(0.6f); // Ensure it's in front of the inventory background

		m_Search_TextField.SetPlaceholderText("Search...");
	}

	InventoryPanel::~InventoryPanel()
	{
		m_EventHandles.clear();
	}

	void InventoryPanel::Render()
	{
		if (IsBackPressed())
		{
			EvtRequestBackNavigation.Trigger(this);
			return;
		}

		if (AreKeyInputsValid() && !m_Search_TextField.IsActive())
		{
			if (EngineContext::Get().Keys->GetKeyState(eAction::OpenInventory).IsPressed)
			{
				EvtRequestBackNavigation.Trigger(this);
				return;
			}
		}

		// Retreve Player State
		std::shared_ptr<Player> player = EngineContext::Get().GetLocalPlayer();

		if (player == nullptr)
		{
			// Create an empty player as placeholder
			player = std::make_shared<Player>("HudPlaceholderPlayer");
		}

		// ----- Gray Background -----
		glm::vec4 backgroundColor = {0.f, 0.f, 0.f, 0.6f}; // RGBA with alpha for transparency
		ColoredBackground::CornerOptions backgroundOptions;
		backgroundOptions.TopLeftCorner = {0, 0};
		backgroundOptions.BottomRightCorner = {s_ScreenWidth, s_ScreenHeight};
		backgroundOptions.ZOffset = 0.52f; // Ensure it's behind the inventory background
		backgroundOptions.Color = backgroundColor;
		ColoredBackground::Render(backgroundOptions);

		// ---- Inventory Background ----
		constexpr float backgroundWidthRatio = 1024.f / 1920.f;
		constexpr float backgroundHeightRatio = 1024.f / 1009.f;
		glm::vec2 backgroundSize = {s_ScreenWidth * backgroundWidthRatio, s_ScreenHeight * backgroundHeightRatio};

		float topLeftXratio = 608.f / 1920.f;
		float topLeftYratio = (195.f - 23.f) / 1009.f;
		float centerXratio = topLeftXratio + (((256.f * 4) / 2.f) / 1920.f);
		float centerYratio = topLeftYratio + (((256.f * 4) / 2.f) / 1009.f);
		glm::vec2 backgroundPos = {s_ScreenWidth * centerXratio, s_ScreenHeight * centerYratio};

		m_InventoryBackground_Sprite.SetPosition(backgroundPos);
		m_InventoryBackground_Sprite.SetSize(backgroundSize);
		m_InventoryBackground_Sprite.Render();

		// ---- Crafting Label ----
		float labelXratio = 996.f / 1920.f;
		float labelYratio = (235.f - 23.f) / 1009.f;
		glm::vec2 labelPos = {s_ScreenWidth * labelXratio, s_ScreenHeight * labelYratio};
		m_Crafting_Label.SetPosition(labelPos);
		m_Crafting_Label.SetTextHeight(s_TextHeight);
		m_Crafting_Label.Render();

		// ---- Constants for Item Rendering ----
		const float slotSizeRatioX = 72.f / 1920.f;
		const float slotSizeRatioY = 72.f / 1009.f;
		const glm::vec2 slotSize = {s_ScreenWidth * slotSizeRatioX, s_ScreenHeight * slotSizeRatioY};
		const glm::vec2 slotPadding = {0.f, 0.f};
		const float slotBorder = 4.f * (s_ScreenWidth / 1920.f);
		const glm::vec2 cursorPosition{s_InputsSnapshot->Mouse.Xpos, s_InputsSnapshot->Mouse.Ypos};

		// ---- Hotbar Item Rendering ----
		const float firstHotbarSlotLeftXborderRatio = 636.f / 1920.f;
		const float firstHotbarSlotTopYborderRatio = (759.f - 23.f) / 1009.f;
		const glm::vec2 firstHotbarSlotTopLeft = {s_ScreenWidth * firstHotbarSlotLeftXborderRatio,
												  s_ScreenHeight * firstHotbarSlotTopYborderRatio};

		const int hoveredHotbarSlotIndex =
			m_HotbarBlockMesh->GetSelectedIndexFromCursorPosition(cursorPosition, firstHotbarSlotTopLeft);
		Inventory hotbar = player->GetHotbar();
		hotbar.SelectedIndex() = hoveredHotbarSlotIndex;
		m_HotbarBlockMesh->SetSlotBorder(slotBorder);
		m_HotbarBlockMesh->SetInventory(hotbar, slotSize, slotPadding);
		if (m_HotbarBlockMesh->IsDirty())
		{
			auto& meshBuilder = EngineContext::Get().WrldRenderer->GetMeshBuilder();
			meshBuilder.UpdateUiBlockMesh(m_HotbarBlockMesh);
		}
		m_HotbarBlockMesh->Render(firstHotbarSlotTopLeft, s_ScreenWidth, s_ScreenHeight);

		// ---- Inventory Item Rendering ----
		const float firstInventorySlotLeftXborderRatio = 636.f / 1920.f;
		const float firstInventorySlotTopYborderRatio = (527.f - 23.f) / 1009.f;
		const glm::vec2 firstInventorySlotTopLeft = {s_ScreenWidth * firstInventorySlotLeftXborderRatio,
													 s_ScreenHeight * firstInventorySlotTopYborderRatio};

		const int hoveredInventorySlotIndex =
			m_InventoryBlockMesh->GetSelectedIndexFromCursorPosition(cursorPosition, firstInventorySlotTopLeft);
		Inventory inventory = player->GetPlayerInventory();
		inventory.SelectedIndex() = hoveredInventorySlotIndex;
		m_InventoryBlockMesh->SetSlotBorder(slotBorder);
		m_InventoryBlockMesh->SetInventory(inventory, slotSize, slotPadding);
		if (m_InventoryBlockMesh->IsDirty())
		{
			auto& meshBuilder = EngineContext::Get().WrldRenderer->GetMeshBuilder();
			meshBuilder.UpdateUiBlockMesh(m_InventoryBlockMesh);
		}
		m_InventoryBlockMesh->Render(firstInventorySlotTopLeft, s_ScreenWidth, s_ScreenHeight);

		// ----- Button Constants -----
		const float pageButtonYratio = (88.f - 23.f) / 1009.f;
		const int pageButtonY = static_cast<int>(std::round(s_ScreenHeight * pageButtonYratio));
		const float pageButtonWidthRatio = 80.f / 1920.f;
		const int pageButtonWidth = static_cast<int>(std::round(s_ScreenWidth * pageButtonWidthRatio));
		const float pageButtonHeightRatio = 80.f / 1009.f;
		const int pageButtonHeight = static_cast<int>(std::round(s_ScreenHeight * pageButtonHeightRatio));
		const glm::ivec2 pageButtonSize = {pageButtonWidth, pageButtonHeight};

		const float prevPageButtonXratio = 1432.f / 1920.f;
		const int prevPageButtonX = static_cast<int>(std::round(s_ScreenWidth * prevPageButtonXratio));
		const float nextPageButtonXratio = 1858.f / 1920.f;
		const int nextPageButtonX = static_cast<int>(std::round(s_ScreenWidth * nextPageButtonXratio));

		// ---- Creative Tab Variables ----
		std::string searchQuery = m_Search_TextField.GetText();
		UpdateFilteredCreativeTabBlockIds(searchQuery);

		// ---- Render Previous Page Button ----
		const glm::ivec2 prevPageButtonPos{prevPageButtonX, pageButtonY};
		m_PreviousPage_Button.SetPosition(prevPageButtonPos);
		m_PreviousPage_Button.SetSize(pageButtonSize);
		m_PreviousPage_Button.Render();

		// ---- Render Next Page Button ----
		const glm::ivec2 nextPageButtonPos{nextPageButtonX, pageButtonY};
		m_NextPage_Button.SetPosition(nextPageButtonPos);
		m_NextPage_Button.SetSize(pageButtonSize);
		m_NextPage_Button.Render();

		// ---- Render darker background behind page indicator ----
		const int bgTopLeftX = static_cast<int>(std::round(prevPageButtonX + (pageButtonWidth / 2.f)));
		const int bgTopLeftY = static_cast<int>(std::round(pageButtonY - (pageButtonHeight / 2.f)));
		const glm::ivec2 pageIndicatorBgTopLeft = {bgTopLeftX, bgTopLeftY};
		const int bgBottomRightX = static_cast<int>(std::round(nextPageButtonX - (pageButtonWidth / 2.f)));
		const int bgBottomRightY = static_cast<int>(std::round(pageButtonY + (pageButtonHeight / 2.f)));
		const glm::ivec2 pageIndicatorBgBottomRight = {bgBottomRightX, bgBottomRightY};

		ColoredBackground::CornerOptions pageIndicatorBgOptions;
		pageIndicatorBgOptions.TopLeftCorner = pageIndicatorBgTopLeft;
		pageIndicatorBgOptions.BottomRightCorner = pageIndicatorBgBottomRight;
		pageIndicatorBgOptions.ZOffset = 0.53f;
		pageIndicatorBgOptions.Color = {0.f, 0.f, 0.f, 0.5f};
		ColoredBackground::Render(pageIndicatorBgOptions);

		// ---- Render Page Indicator Label ----
		const int pageIndicatorX =
			static_cast<int>(std::round((nextPageButtonX - prevPageButtonX) / 2.f + prevPageButtonX));
		const int pageIndicatorY = pageButtonY;
		const glm::ivec2 pageIndicatorPos = {pageIndicatorX, pageIndicatorY};
		const std::string pageIndicatorText =
			std::to_string(m_CurrentPageIndex + 1) + "/" + std::to_string(m_MaxPageIndex + 1);
		m_PageIndicator_Label.SetText(pageIndicatorText);
		m_PageIndicator_Label.SetPosition(pageIndicatorPos);
		m_PageIndicator_Label.SetTextHeight(s_TextHeight);
		m_PageIndicator_Label.Render();

		// ---- Render Search Text Field ----
		const float searchFieldYratio = (972.f - 23.f) / 1009.f;
		const int searchFieldY = static_cast<int>(std::round(s_ScreenHeight * searchFieldYratio));
		const glm::ivec2 searchFieldPos = {pageIndicatorX, searchFieldY};
		// From left of first button, to right of second.
		const int searchFieldWidth = static_cast<int>(
			std::round((nextPageButtonX + (pageButtonWidth / 2.f)) - (prevPageButtonX - (pageButtonWidth / 2.f))));
		m_Search_TextField.SetPosition(searchFieldPos);
		m_Search_TextField.SetSize({searchFieldWidth, pageButtonHeight});
		m_Search_TextField.Render();

		// ----- Render Creative Tab Blocks -----
		const float firstCreativeTabSlotLeftXborderRatio = 1392.f / 1920.f;
		const int firstCreativeTabSlotLeftXborder =
			static_cast<int>(std::round(s_ScreenWidth * firstCreativeTabSlotLeftXborderRatio));
		const float firstCreativeTabSlotTopYborderRatio = (136.f - 23.f) / 1009.f;
		const int firstCreativeTabSlotTopYborder =
			static_cast<int>(std::round(s_ScreenHeight * firstCreativeTabSlotTopYborderRatio));
		const glm::vec2 firstCreativeTabSlotBorder = {firstCreativeTabSlotLeftXborder, firstCreativeTabSlotTopYborder};

		const int hoveredCreativeSlotIndex =
			m_CreativeBlockMesh->GetSelectedIndexFromCursorPosition(cursorPosition, firstCreativeTabSlotBorder);
		m_CreativeTabInventory.SelectedIndex() = hoveredCreativeSlotIndex;
		const int slotsPerPage = m_CreativeTabInventory.Rows() * m_CreativeTabInventory.Columns();
		int startIndex = m_CurrentPageIndex * slotsPerPage;
		for (int i = 0; i < slotsPerPage; i++)
		{
			if (startIndex + i < static_cast<int>(m_FilteredCreativeTabBlockIds.size()))
			{
				m_CreativeTabInventory.At(i) = m_FilteredCreativeTabBlockIds[startIndex + i];
			}
			else
			{
				m_CreativeTabInventory.At(i) = BlockId::Air; // Empty slot
			}
		}

		m_CreativeBlockMesh->SetSlotBorder(slotBorder);
		m_CreativeBlockMesh->SetInventory(m_CreativeTabInventory, slotSize, slotPadding);
		if (m_CreativeBlockMesh->IsDirty())
		{
			auto& meshBuilder = EngineContext::Get().WrldRenderer->GetMeshBuilder();
			meshBuilder.UpdateUiBlockMesh(m_CreativeBlockMesh);
		}
		m_CreativeBlockMesh->Render(firstCreativeTabSlotBorder, s_ScreenWidth, s_ScreenHeight);

		// ----- Renders armor slots sprites (not inventory blocks) -----
		const float firstArmorSlotXleftRatio = 636.f / 1920.f;
		const int firstArmorSlotXleft = static_cast<int>(std::round(s_ScreenWidth * firstArmorSlotXleftRatio));
		const float firstArmorSlotYtopRatio = (223.f - 23.f) / 1009.f;
		const int firstArmorSlotYtop = static_cast<int>(std::round(s_ScreenHeight * firstArmorSlotYtopRatio));
		const int firstArmorYcenter = static_cast<int>(glm::round(firstArmorSlotYtop + (slotSize.y / 2)));
		glm::ivec2 armorSlotCenter = {firstArmorSlotXleft + slotSize.x / 2, firstArmorYcenter};
		const glm::vec2 spriteSlotSize = slotSize - glm::vec2{2 * slotBorder};
		// Helmet Slot
		m_ArmorHelmet_Sprite.SetPosition(armorSlotCenter);
		m_ArmorHelmet_Sprite.SetSize(slotSize);
		m_ArmorHelmet_Sprite.Render();

		// Chestplate Slot
		armorSlotCenter.y = firstArmorYcenter + static_cast<int>(glm::round(slotSize.y));
		m_ArmorChestplate_Sprite.SetPosition(armorSlotCenter);
		m_ArmorChestplate_Sprite.SetSize(spriteSlotSize);
		m_ArmorChestplate_Sprite.Render();

		// Leggings Slot
		armorSlotCenter.y = firstArmorYcenter + static_cast<int>(glm::round(slotSize.y * 2));
		m_ArmorLeggings_Sprite.SetPosition(armorSlotCenter);
		m_ArmorLeggings_Sprite.SetSize(spriteSlotSize);
		m_ArmorLeggings_Sprite.Render();

		// Boots Slot
		armorSlotCenter.y = firstArmorYcenter + static_cast<int>(glm::round(slotSize.y * 3));
		m_ArmorBoots_Sprite.SetPosition(armorSlotCenter);
		m_ArmorBoots_Sprite.SetSize(spriteSlotSize);
		m_ArmorBoots_Sprite.Render();

		// ----- Render Armor Slots Inventory Blocks -----
		// Player has no armor inventory yet.
		const glm::vec2 firstArmorSlotTopLeft = {firstArmorSlotXleft, firstArmorSlotYtop};
		const int hoveredArmorSlotIndex =
			m_ArmorBlockMesh->GetSelectedIndexFromCursorPosition(cursorPosition, firstArmorSlotTopLeft);
		Inventory armorInventory(4, 1);
		armorInventory.SelectedIndex() = hoveredArmorSlotIndex;
		m_ArmorBlockMesh->SetSlotBorder(slotBorder);
		m_ArmorBlockMesh->SetInventory(armorInventory, slotSize, slotPadding);
		if (m_ArmorBlockMesh->IsDirty())
		{
			auto& meshBuilder = EngineContext::Get().WrldRenderer->GetMeshBuilder();
			meshBuilder.UpdateUiBlockMesh(m_ArmorBlockMesh);
		}
		m_ArmorBlockMesh->Render(firstArmorSlotTopLeft, s_ScreenWidth, s_ScreenHeight);

		// ----- Render Offhand Slot Sprite -----
		const float offhandSlotXleftRatio = 912.f / 1920.f;
		const int offhandSlotXleft = static_cast<int>(std::round(s_ScreenWidth * offhandSlotXleftRatio));
		const float offhandSlotYtopRatio = (440.f - 23.f) / 1009.f;
		const int offhandSlotYtop = static_cast<int>(std::round(s_ScreenHeight * offhandSlotYtopRatio));
		glm::ivec2 offhandSlotCenter = {offhandSlotXleft + slotSize.x / 2, offhandSlotYtop + slotSize.y / 2};
		m_Offhand_Sprite.SetPosition(offhandSlotCenter);
		m_Offhand_Sprite.SetSize(spriteSlotSize);
		m_Offhand_Sprite.Render();

		// ---- Render Offhand Slot Inventory Block ----
		const glm::vec2 offhandSlotTopLeft = {offhandSlotXleft, offhandSlotYtop};
		const int hoveredOffhandSlotIndex =
			m_OffhandBlockMesh->GetSelectedIndexFromCursorPosition(cursorPosition, offhandSlotTopLeft);
		Inventory offhandInventory(1, 1);
		offhandInventory.SelectedIndex() = hoveredOffhandSlotIndex;
		m_OffhandBlockMesh->SetSlotBorder(slotBorder);
		m_OffhandBlockMesh->SetInventory(offhandInventory, slotSize, slotPadding);
		if (m_OffhandBlockMesh->IsDirty())
		{
			auto& meshBuilder = EngineContext::Get().WrldRenderer->GetMeshBuilder();
			meshBuilder.UpdateUiBlockMesh(m_OffhandBlockMesh);
		}
		m_OffhandBlockMesh->Render(offhandSlotTopLeft, s_ScreenWidth, s_ScreenHeight);

		// ----- Render Crafting Grid Slots -----
		const float firstCraftingGridSlotXleftRatio = 996.f / 1920.f;
		const int firstCraftingGridSlotXleft = static_cast<int>(std::round(s_ScreenWidth * firstCraftingGridSlotXleftRatio));
		const float firstCraftingGridSlotYtopRatio = (263.f - 23.f) / 1009.f;
		const int firstCraftingGridSlotYtop = static_cast<int>(std::round(s_ScreenHeight * firstCraftingGridSlotYtopRatio));
		const glm::vec2 firstCraftingGridSlotTopLeft = {firstCraftingGridSlotXleft, firstCraftingGridSlotYtop};
		const int hoveredCraftingGridSlotIndex =
			m_CraftingGridBlockMesh->GetSelectedIndexFromCursorPosition(cursorPosition, firstCraftingGridSlotTopLeft);
		m_CraftingInventory.SelectedIndex() = hoveredCraftingGridSlotIndex;
		m_CraftingGridBlockMesh->SetSlotBorder(slotBorder);
		m_CraftingGridBlockMesh->SetInventory(m_CraftingInventory, slotSize, slotPadding);
		if (m_CraftingGridBlockMesh->IsDirty())
		{
			auto& meshBuilder = EngineContext::Get().WrldRenderer->GetMeshBuilder();
			meshBuilder.UpdateUiBlockMesh(m_CraftingGridBlockMesh);
		}
		m_CraftingGridBlockMesh->Render(firstCraftingGridSlotTopLeft, s_ScreenWidth, s_ScreenHeight);

		// ----- Render Crafting Output Slot -----
		const float craftingOutputSlotXleftRatio = 1220.f / 1920.f;
		const int craftingOutputSlotXleft = static_cast<int>(std::round(s_ScreenWidth * craftingOutputSlotXleftRatio));
		const float craftingOutputSlotYtopRatio = (303.f - 23.f) / 1009.f;
		const int craftingOutputSlotYtop = static_cast<int>(std::round(s_ScreenHeight * craftingOutputSlotYtopRatio));
		const glm::vec2 craftingOutputSlotTopLeft = {craftingOutputSlotXleft, craftingOutputSlotYtop};
		const int hoveredCraftingOutputSlotIndex =
			m_CraftingOutputBlockMesh->GetSelectedIndexFromCursorPosition(cursorPosition, craftingOutputSlotTopLeft);
		m_CraftingOutputInventory.SelectedIndex() = hoveredCraftingOutputSlotIndex;
		m_CraftingOutputBlockMesh->SetSlotBorder(slotBorder);
		m_CraftingOutputBlockMesh->SetInventory(m_CraftingOutputInventory, slotSize, slotPadding);
		if (m_CraftingOutputBlockMesh->IsDirty())
		{
			auto& meshBuilder = EngineContext::Get().WrldRenderer->GetMeshBuilder();
			meshBuilder.UpdateUiBlockMesh(m_CraftingOutputBlockMesh);
		}
		m_CraftingOutputBlockMesh->Render(craftingOutputSlotTopLeft, s_ScreenWidth, s_ScreenHeight);

		// ---- Key Drop Input Handling ----
		bool dropKeyPressed = EngineContext::Get().Keys->GetKeyState(eAction::DropItem).IsPressed;

		// ---- Tooltip Rendering for Hotbar (if needed) ----
		if (hoveredHotbarSlotIndex != -1)
		{
			BlockId hoveredBlockId = hotbar.At(hoveredHotbarSlotIndex);
			if (hoveredBlockId != BlockId::Air) // Only show tooltip for non-empty slots
			{
				if (dropKeyPressed)
				{
					hotbar.At(hoveredHotbarSlotIndex) = BlockId::Air; // Simulate dropping the item by clearing the slot
					player->SetHotbar(hotbar); // Update the player's hotbar with the modified inventory
				}
				else
				{
					const std::string tooltipText = BuildTooltipText(hoveredBlockId);
					m_Tooltip.SetText(tooltipText);
					m_Tooltip.SetTextHeight(s_TextHeight);
					m_Tooltip.SetPosition(cursorPosition);
					m_Tooltip.Render();
				}
			}
		}

		// ---- Tooltip Rendering for Inventory (if needed) ----
		if (hoveredInventorySlotIndex != -1)
		{
			BlockId hoveredBlockId = inventory.At(hoveredInventorySlotIndex);
			if (hoveredBlockId != BlockId::Air) // Only show tooltip for non-empty slots
			{
				if (dropKeyPressed)
				{
					inventory.At(hoveredInventorySlotIndex) = BlockId::Air; // Clear the slot
					player->SetPlayerInventory(inventory); // Update the player's inventory with the modified inventory
				}
				else
				{
					const std::string tooltipText = BuildTooltipText(hoveredBlockId);
					m_Tooltip.SetText(tooltipText);
					m_Tooltip.SetTextHeight(s_TextHeight);
					m_Tooltip.SetPosition(cursorPosition);
					m_Tooltip.Render();
				}
			}
		}

		// ---- Tooltip Rendering for Creative Tab (if needed) ----
		if (hoveredCreativeSlotIndex != -1)
		{
			BlockId hoveredBlockId = m_CreativeTabInventory.At(hoveredCreativeSlotIndex);
			if (hoveredBlockId != BlockId::Air) // Only show tooltip for non-empty slots
			{
				const std::string tooltipText = BuildTooltipText(hoveredBlockId);
				m_Tooltip.SetText(tooltipText);
				m_Tooltip.SetTextHeight(s_TextHeight);
				m_Tooltip.SetPosition(cursorPosition);
				m_Tooltip.Render();
			}
		}
	}

	void InventoryPanel::Initialize()
	{
		m_InventoryBackground_Sprite.Initialize();
		m_Crafting_Label.Initialize();
		m_Tooltip.Initialize();
		m_PreviousPage_Button.Initialize();
		m_NextPage_Button.Initialize();
		m_PageIndicator_Label.Initialize();
		m_Search_TextField.Initialize();
		m_ArmorHelmet_Sprite.Initialize();
		m_ArmorChestplate_Sprite.Initialize();
		m_ArmorLeggings_Sprite.Initialize();
		m_ArmorBoots_Sprite.Initialize();
		m_Offhand_Sprite.Initialize();

		SetInitState(true);
	}

	void InventoryPanel::Delete()
	{
		m_InventoryBackground_Sprite.Delete();
		m_Crafting_Label.Delete();
		m_Tooltip.Delete();
		m_PreviousPage_Button.Delete();
		m_NextPage_Button.Delete();
		m_PageIndicator_Label.Delete();
		m_Search_TextField.Delete();
		m_ArmorHelmet_Sprite.Delete();
		m_ArmorChestplate_Sprite.Delete();
		m_ArmorLeggings_Sprite.Delete();
		m_ArmorBoots_Sprite.Delete();
		m_Offhand_Sprite.Delete();

		m_HotbarBlockMesh->Delete();
		m_InventoryBlockMesh->Delete();
		m_CreativeBlockMesh->Delete();
		m_ArmorBlockMesh->Delete();
		m_OffhandBlockMesh->Delete();
		m_CraftingGridBlockMesh->Delete();
		m_CraftingOutputBlockMesh->Delete();

		SetDeletedState(true);
	}

	void InventoryPanel::ReloadTextures()
	{
		m_InventoryBackground_Sprite.ReloadTextures();
		m_Crafting_Label.ReloadTextures();
		m_Tooltip.ReloadTextures();
		m_PreviousPage_Button.ReloadTextures();
		m_NextPage_Button.ReloadTextures();
		m_PageIndicator_Label.ReloadTextures();
		m_Search_TextField.ReloadTextures();
		m_ArmorHelmet_Sprite.ReloadTextures();
		m_ArmorChestplate_Sprite.ReloadTextures();
		m_ArmorLeggings_Sprite.ReloadTextures();
		m_ArmorBoots_Sprite.ReloadTextures();
		m_Offhand_Sprite.ReloadTextures();

		m_HotbarBlockMesh->SetDirty(true);
		m_InventoryBlockMesh->SetDirty(true);
		m_CreativeBlockMesh->SetDirty(true);
		m_ArmorBlockMesh->SetDirty(true);
		m_OffhandBlockMesh->SetDirty(true);
		m_CraftingGridBlockMesh->SetDirty(true);
		m_CraftingOutputBlockMesh->SetDirty(true);
	}

	const std::vector<BlockId>& InventoryPanel::GetCreativeTabBlockIds()
	{
		static std::vector<BlockId> creativeTabBlockIds = []
		{
			std::vector<BlockId> blockIds;
			blockIds.reserve(static_cast<size_t>(BlockId::Count));
			for (int blockIdInt = 1; blockIdInt < static_cast<int>(BlockId::Count); blockIdInt++)
			{
				blockIds.push_back(static_cast<BlockId>(blockIdInt));
			}
			return blockIds;
		}();

		return creativeTabBlockIds;
	}

	void InventoryPanel::UpdateFilteredCreativeTabBlockIds(const std::string& search)
	{
		// If the search query hasn't changed, no need to update
		if (m_LastSearchQuery == search)
			return;

		// If search is empty, show all blocks
		if (search.empty())
		{
			m_FilteredCreativeTabBlockIds = GetCreativeTabBlockIds();
			m_LastSearchQuery = search;

			// Update Max Page Index based on full list
			m_MaxPageIndex = ComputeMaxPageIndex(static_cast<int>(m_FilteredCreativeTabBlockIds.size()));

			if (m_CurrentPageIndex > m_MaxPageIndex)
				m_CurrentPageIndex = 0;

			return;
		}

		m_FilteredCreativeTabBlockIds.clear();

		auto toLower = [](std::string s)
		{
			std::transform(
				s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(::tolower(c)); });
			return s;
		};

		const std::string searchLower = toLower(search);

		for (const BlockId blockId : GetCreativeTabBlockIds())
		{
			const std::string blockNameLower = toLower(BlockIds::GetName(blockId));
			if (blockNameLower.find(searchLower) != std::string::npos)
			{
				m_FilteredCreativeTabBlockIds.push_back(blockId);
			}
		}

		// Update Max Page Index based on filtered results
		m_MaxPageIndex = ComputeMaxPageIndex(static_cast<int>(m_FilteredCreativeTabBlockIds.size()));

		if (m_CurrentPageIndex > m_MaxPageIndex)
			m_CurrentPageIndex = 0;

		m_LastSearchQuery = search;
	}

	int InventoryPanel::ComputeMaxPageIndex(int itemCount) const
	{
		const int slotsPerPage = m_CreativeTabInventory.Rows() * m_CreativeTabInventory.Columns();
		const int totalPages = static_cast<int>(std::ceil(static_cast<float>(itemCount) / slotsPerPage));
		const int maxPageIndex = totalPages - 1;

		return std::max(0, maxPageIndex);
	}

	void InventoryPanel::SubscribeToControlEvents()
	{
		m_EventHandles.push_back(m_PreviousPage_Button.EvtClick.Subscribe([this](const Button& button)
																		  { Handle_PreviousPageButtonClick(button); }));

		m_EventHandles.push_back(
			m_NextPage_Button.EvtClick.Subscribe([this](const Button& button) { Handle_NextPageButtonClick(button); }));
	}

	void InventoryPanel::Handle_PreviousPageButtonClick(const Button& button)
	{
		(void) button;
		if (m_MaxPageIndex == 0)
			return;

		m_CurrentPageIndex = (m_CurrentPageIndex - 1 + (m_MaxPageIndex + 1)) % (m_MaxPageIndex + 1);
	}

	void InventoryPanel::Handle_NextPageButtonClick(const Button& button)
	{
		(void) button;
		if (m_MaxPageIndex == 0)
			return;

		m_CurrentPageIndex = (m_CurrentPageIndex + 1) % (m_MaxPageIndex + 1);
	}

} // namespace onion::voxel
