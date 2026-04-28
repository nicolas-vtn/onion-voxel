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
		  m_Crafting_Label("Crafting_Label"), m_Tooltip("InventoryTooltip")
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
	}

	void InventoryPanel::Initialize()
	{
		m_InventoryBackground_Sprite.Initialize();
		m_Crafting_Label.Initialize();
		m_Tooltip.Initialize();

		SetInitState(true);
	}

	void InventoryPanel::Delete()
	{
		m_InventoryBackground_Sprite.Delete();
		m_Crafting_Label.Delete();
		m_Tooltip.Delete();

		SetDeletedState(true);
	}

	void InventoryPanel::ReloadTextures()
	{
		m_InventoryBackground_Sprite.ReloadTextures();
		m_Crafting_Label.ReloadTextures();
		m_Tooltip.ReloadTextures();
	}

	void InventoryPanel::SubscribeToControlEvents()
	{
		// No button controls yet — Escape (s_IsBackPressed) is handled directly in Render()
	}

} // namespace onion::voxel
