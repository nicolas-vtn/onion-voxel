#include "InventoryPanel.hpp"

#include <renderer/gui/colored_background/ColoredBackground.hpp>

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
