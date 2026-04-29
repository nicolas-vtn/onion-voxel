#include "InventoryPanel.hpp"

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
		  m_PageIndicator_Label("PageIndicator_Label"), m_Search_TextField("Search_TextField")
	{
		SubscribeToControlEvents();

		m_InventoryBackground_Sprite.SetZOffset(0.55f); // Ensure it's in front of the Hotbar

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
		if (s_IsBackPressed)
		{
			EvtRequestBackNavigation.Trigger(this);
			return;
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
		const int bgTopLeftX = prevPageButtonX + (pageButtonWidth / 2.f);
		const int bgTopLeftY = pageButtonY - (pageButtonHeight / 2.f);
		const glm::ivec2 pageIndicatorBgTopLeft = {bgTopLeftX, bgTopLeftY};
		const int bgBottomRightX = nextPageButtonX - (pageButtonWidth / 2.f);
		const int bgBottomRightY = pageButtonY + (pageButtonHeight / 2.f);
		const glm::ivec2 pageIndicatorBgBottomRight = {bgBottomRightX, bgBottomRightY};

		ColoredBackground::CornerOptions pageIndicatorBgOptions;
		pageIndicatorBgOptions.TopLeftCorner = pageIndicatorBgTopLeft;
		pageIndicatorBgOptions.BottomRightCorner = pageIndicatorBgBottomRight;
		pageIndicatorBgOptions.ZOffset = 0.53f;
		pageIndicatorBgOptions.Color = {0.f, 0.f, 0.f, 0.5f};
		ColoredBackground::Render(pageIndicatorBgOptions);

		// ---- Render Page Indicator Label ----
		const int pageIndicatorX = (nextPageButtonX - prevPageButtonX) / 2.f + prevPageButtonX;
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
		const int searchFieldWidth =
			(nextPageButtonX + (pageButtonWidth / 2.f)) - (prevPageButtonX - (pageButtonWidth / 2.f));
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

		// ---- Tooltip Rendering for Hotbar (if needed) ----
		if (hoveredHotbarSlotIndex != -1)
		{
			BlockId hoveredBlockId = hotbar.At(hoveredHotbarSlotIndex);
			if (hoveredBlockId != BlockId::Air) // Only show tooltip for non-empty slots
			{
				const std::string tooltipText = BuildTooltipText(hoveredBlockId);
				m_Tooltip.SetText(tooltipText);
				m_Tooltip.SetTextHeight(s_TextHeight);
				m_Tooltip.SetPosition(cursorPosition);
				m_Tooltip.Render();
			}
		}

		// ---- Tooltip Rendering for Inventory (if needed) ----
		if (hoveredInventorySlotIndex != -1)
		{
			BlockId hoveredBlockId = inventory.At(hoveredInventorySlotIndex);
			if (hoveredBlockId != BlockId::Air) // Only show tooltip for non-empty slots
			{
				const std::string tooltipText = BuildTooltipText(hoveredBlockId);
				m_Tooltip.SetText(tooltipText);
				m_Tooltip.SetTextHeight(s_TextHeight);
				m_Tooltip.SetPosition(cursorPosition);
				m_Tooltip.Render();
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
	}

	const std::vector<BlockId>& InventoryPanel::GetCreativeTabBlockIds()
	{
		static std::vector<BlockId> creativeTabBlockIds = []
		{
			std::vector<BlockId> blockIds;
			blockIds.reserve(static_cast<size_t>(BlockId::Count) + 1);
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
			m_MaxPageIndex =
				static_cast<int>(std::ceil(static_cast<float>(m_FilteredCreativeTabBlockIds.size()) /
										   (m_CreativeTabInventory.Rows() * m_CreativeTabInventory.Columns()))) -
				1;

			if (m_CurrentPageIndex > m_MaxPageIndex)
				m_CurrentPageIndex = 0;

			return;
		}

		m_FilteredCreativeTabBlockIds.clear();

		for (const BlockId blockId : GetCreativeTabBlockIds())
		{
			const std::string blockName = BlockIds::GetName(blockId);
			if (blockName.find(search) != std::string::npos)
			{
				m_FilteredCreativeTabBlockIds.push_back(blockId);
			}
		}

		// Update Max Page Index based on filtered results
		m_MaxPageIndex =
			static_cast<int>(std::ceil(static_cast<float>(m_FilteredCreativeTabBlockIds.size()) /
									   (m_CreativeTabInventory.Rows() * m_CreativeTabInventory.Columns()))) -
			1;

		if (m_CurrentPageIndex > m_MaxPageIndex)
			m_CurrentPageIndex = 0;

		m_LastSearchQuery = search;
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
		if (m_MaxPageIndex == 0)
			return;

		m_CurrentPageIndex = (m_CurrentPageIndex - 1 + (m_MaxPageIndex + 1)) % (m_MaxPageIndex + 1);
	}

	void InventoryPanel::Handle_NextPageButtonClick(const Button& button)
	{
		if (m_MaxPageIndex == 0)
			return;

		m_CurrentPageIndex = (m_CurrentPageIndex + 1) % (m_MaxPageIndex + 1);
	}

} // namespace onion::voxel
