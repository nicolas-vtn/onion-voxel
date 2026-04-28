#include "InventoryPanel.hpp"

#include <renderer/gui/colored_background/ColoredBackground.hpp>

#include <renderer/world_renderer/WorldRenderer.hpp>

namespace onion::voxel
{
	InventoryPanel::InventoryPanel(const std::string& name)
		: GuiElement(name), m_InventoryBackground_Sprite(
								"InventoryBackground_Sprite", s_PathInventoryBackground, Sprite::eOrigin::ResourcePack),
		  m_Crafting_Label("Crafting_Label")
	{
		SubscribeToControlEvents();

		m_InventoryBackground_Sprite.SetZOffset(0.55f); // Ensure it's in front of the Hotbar

		m_Crafting_Label.SetZOffset(0.6f); // Ensure it's in front of the inventory background
		m_Crafting_Label.SetText("Crafting");
		glm::vec3 craftingTextColor = {63 / 255.f, 63 / 255.f, 63 / 255.f}; // Light gray color
		m_Crafting_Label.SetCustomTextColor(craftingTextColor);
		m_Crafting_Label.EnableShadow(false);
		m_Crafting_Label.SetTextAlignment(Font::eTextAlignment::Left);
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

		// ---- Hotbar Item Rendering ----
		float blockSlotSizeRatioX = 64.f / 1920.f;
		float blockSlotSizeRatioY = 64.f / 1009.f;
		glm::vec2 slotSize = {s_ScreenWidth * blockSlotSizeRatioX, s_ScreenHeight * blockSlotSizeRatioY};
		float slotPaddingRatioX = 8.f / 1920.f;
		float slotPaddingRatioY = 8.f / 1009.f;
		glm::vec2 slotPadding = {s_ScreenWidth * slotPaddingRatioX, s_ScreenHeight * slotPaddingRatioY};
		float firstSlotLeftXborderRatio = 640.f / 1920.f;
		float firstSlotTopYborderRatio = (763.f - 23.f) / 1009.f;
		glm::vec2 firstSlotTopLeft = {s_ScreenWidth * firstSlotLeftXborderRatio,
									  s_ScreenHeight * firstSlotTopYborderRatio};
		m_HotbarBlockMesh->SetInventory(player->GetHotbar(), slotSize, slotPadding);
		if (m_HotbarBlockMesh->IsDirty())
		{
			auto& meshBuilder = EngineContext::Get().WrldRenderer->GetMeshBuilder();
			meshBuilder.UpdateUiBlockMesh(m_HotbarBlockMesh);
		}
		m_HotbarBlockMesh->Render(firstSlotTopLeft, s_ScreenWidth, s_ScreenHeight);
	}

	void InventoryPanel::Initialize()
	{
		m_InventoryBackground_Sprite.Initialize();
		m_Crafting_Label.Initialize();

		SetInitState(true);
	}

	void InventoryPanel::Delete()
	{
		m_InventoryBackground_Sprite.Delete();
		m_Crafting_Label.Delete();

		SetDeletedState(true);
	}

	void InventoryPanel::ReloadTextures()
	{
		m_InventoryBackground_Sprite.ReloadTextures();
		m_Crafting_Label.ReloadTextures();
	}

	void InventoryPanel::SubscribeToControlEvents()
	{
		// No button controls yet — Escape (s_IsBackPressed) is handled directly in Render()
	}

} // namespace onion::voxel
