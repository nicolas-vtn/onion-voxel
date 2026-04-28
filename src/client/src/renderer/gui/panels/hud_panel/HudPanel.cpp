#include "HudPanel.hpp"

#include <renderer/debug_draws/DebugDraws.hpp>
#include <renderer/world_renderer/WorldRenderer.hpp>

namespace onion::voxel
{
	HudPanel::HudPanel(const std::string& name)
		: GuiElement(name), m_Hotbar_Sprite("Hotbar_Sprite", s_PathHotbar, Sprite::eOrigin::ResourcePack),
		  m_HotbarSelection_Sprite("HotbarSelection_Sprite", s_PathHotbarSelection, Sprite::eOrigin::ResourcePack),
		  m_HeartFull_Sprite("HeartFull_Sprite", s_PathHeartFull, Sprite::eOrigin::ResourcePack),
		  m_HeartHalf_Sprite("HeartHalf_Sprite", s_PathHeartHalf, Sprite::eOrigin::ResourcePack),
		  m_HungerFull_Sprite("HungerFull_Sprite", s_PathHungerFull, Sprite::eOrigin::ResourcePack),
		  m_HungerHalf_Sprite("HungerHalf_Sprite", s_PathHungerHalf, Sprite::eOrigin::ResourcePack),
		  m_ExperienceBarBackground_Sprite(
			  "ExperienceBarBackground_Sprite", s_PathExperienceBarBackground, Sprite::eOrigin::ResourcePack),
		  m_ExperienceBarProgress_Sprite(
			  "ExperienceBarProgress_Sprite", s_PathExperienceBarProgress, Sprite::eOrigin::ResourcePack),
		  m_HeartContainer_Sprite("HeartContainer_Sprite", s_PathHeartContainer, Sprite::eOrigin::ResourcePack),
		  m_HungerEmpty_Sprite("HungerEmpty_Sprite", s_PathHungerEmpty, Sprite::eOrigin::ResourcePack),
		  m_ExperienceLevel_Label("ExperienceLevel_Label")
	{
		m_ExperienceBarBackground_Sprite.SetZOffset(0.4f);
		m_ExperienceBarProgress_Sprite.SetZOffset(0.45f);

		m_ExperienceLevel_Label.SetZOffset(0.5f);
		m_ExperienceLevel_Label.SetTextColor(Font::eColor::Green);
		m_ExperienceLevel_Label.SetTextAlignment(Font::eTextAlignment::Center);

		m_UiBlockMesh->SetRenderSelectedHighlight(false);
	}

	void HudPanel::Render()
	{
		// Retreve Player State
		std::shared_ptr<Player> player = EngineContext::Get().GetLocalPlayer();

		if (player == nullptr)
		{
			// Create an empty player as placeholder
			player = std::make_shared<Player>("HudPlaceholderPlayer");
		}

		// ---- Handle Hotbar Scrolling Input ----
		double rawScroll = s_InputsSnapshot->Mouse.ScrollYoffset;

		int scroll = 0;
		if (rawScroll > 0.0)
			scroll = 1;
		else if (rawScroll < 0.0)
			scroll = -1;

		bool spacePressed = EngineContext::Get().Inputs->IsKeyPressed(Key::Space);
		bool shiftPressed = EngineContext::Get().Inputs->IsKeyPressed(Key::LeftShift) ||
			EngineContext::Get().Inputs->IsKeyPressed(Key::RightShift);

		// This combinaison is used to lock the hotbar scroll when Accelerating Fly Speed.
		bool bypassScroll = spacePressed && shiftPressed;

		if (scroll != 0 && !bypassScroll)
		{
			auto playerHotbar = player->GetHotbar();
			const int maxSlots = playerHotbar.Rows() * playerHotbar.Columns();

			int current = static_cast<int>(playerHotbar.SelectedIndex());

			// Proper modulo wrapping (handles negative values correctly)
			int newSlot = (current - scroll) % maxSlots;
			if (newSlot < 0)
				newSlot += maxSlots;

			playerHotbar.SelectedIndex() = static_cast<uint8_t>(newSlot);
			player->SetHotbar(playerHotbar);
		}

		// ---- Constants ----
		int screenCenterX = static_cast<int>(std::round(s_ScreenWidth * 0.5f));
		int screenBottom = static_cast<int>(s_ScreenHeight);

		// ---- Hotbar (bottom-center) ----
		float hotbarHeightRatio = 86.f / 1009.f;
		int hotbarHeight = static_cast<int>(s_ScreenHeight * hotbarHeightRatio);
		float hotbarWidthRatio = 725.f / 1920.f;
		int hotbarWidth = static_cast<int>(s_ScreenWidth * hotbarWidthRatio);
		glm::ivec3 hotbarCenter = {screenCenterX, screenBottom - hotbarHeight / 2, 0};
		m_Hotbar_Sprite.SetPosition(hotbarCenter);
		m_Hotbar_Sprite.SetSize({hotbarWidth, hotbarHeight});
		m_Hotbar_Sprite.Render();

		// ---- Hotbar Selection (first slot, overlaid on hotbar) ----
		float slotSizeRatioX = 95.f / 1920.f;
		float slotSizeRatioY = 95.f / 1009.f;
		int slotSizeX = static_cast<int>(s_ScreenWidth * slotSizeRatioX);
		int slotSizeY = static_cast<int>(s_ScreenHeight * slotSizeRatioY);
		float hotbarBorderRatio = 4.f / 1920.f;
		float spacingRatio = (hotbarWidthRatio - (2 * hotbarBorderRatio)) / 9.f;
		float leftHotbarEdgeRatio = 0.5f - (hotbarWidthRatio - hotbarBorderRatio) * 0.5f;
		int selectedSlotIndex = player->GetHotbar().SelectedIndex();
		float selectionXratio = leftHotbarEdgeRatio + (spacingRatio / 2.f) + (spacingRatio * selectedSlotIndex);
		glm::vec2 hotbarSelectionPos = {s_ScreenWidth * selectionXratio, screenBottom - hotbarHeight / 2};
		m_HotbarSelection_Sprite.SetPosition(hotbarSelectionPos);
		m_HotbarSelection_Sprite.SetSize({slotSizeX, slotSizeY});
		m_HotbarSelection_Sprite.Render();

		// ---- Hotbar Item Rendering (overlaid on hotbar) ----
		float blockSlotSizeRatioX = 64.f / 1920.f;
		float blockSlotSizeRatioY = 64.f / 1009.f;
		glm::vec2 slotSize = {s_ScreenWidth * blockSlotSizeRatioX, s_ScreenHeight * blockSlotSizeRatioY};
		float slotPaddingRatioX = 16.f / 1920.f;
		glm::vec2 slotPadding = {s_ScreenWidth * slotPaddingRatioX, 0.f};
		float firstSlotLeftXborderRatio = 608.f / 1920.f;
		float firstSlotTopYborderRatio = (956.f - 23.f) / 1009.f;
		glm::vec2 firstSlotTopLeft = {s_ScreenWidth * firstSlotLeftXborderRatio,
									  s_ScreenHeight * firstSlotTopYborderRatio};
		m_UiBlockMesh->SetInventory(player->GetHotbar(), slotSize, slotPadding);
		if (m_UiBlockMesh->IsDirty())
		{
			auto& meshBuilder = EngineContext::Get().WrldRenderer->GetMeshBuilder();
			meshBuilder.UpdateUiBlockMesh(m_UiBlockMesh);
		}
		m_UiBlockMesh->Render(firstSlotTopLeft, s_ScreenWidth, s_ScreenHeight);

		// ---- Health (hearts, bottom-left above hotbar) ----
		float health = player->GetHealth().CurrentHealth;
		float healthSizeX = (36.f / 1920.f) * s_ScreenWidth;
		float healthSizeY = (36.f / 1009.f) * s_ScreenHeight;
		glm::vec2 healthSize = {healthSizeX, healthSizeY};
		float firstHealthRatioX = 614.f / 1920.f;
		float firstHealthRatioY = 874.f / 1009.f;
		glm::vec2 firstHealthPos = {s_ScreenWidth * firstHealthRatioX, s_ScreenHeight * firstHealthRatioY};

		// Health Containers
		m_HeartContainer_Sprite.SetSize(healthSize);
		float magicOffsetX = (1.f / 9.f) * healthSizeX;
		for (int i = 0; i < 10; i++)
		{
			glm::vec2 heartPos = {firstHealthPos.x + i * (healthSize.x - magicOffsetX), firstHealthPos.y};
			m_HeartContainer_Sprite.SetPosition(heartPos);
			m_HeartContainer_Sprite.Render();
		}

		// Full Hearts
		int halfUnits = static_cast<int>(std::floor(health)); // number of half-hearts
		if (health > 0.f && halfUnits == 0)
		{
			halfUnits = 1; // Ensure at least a half-heart is shown for any positive health
		}
		int fullHearts = halfUnits / 2;
		m_HeartFull_Sprite.SetSize(healthSize);
		for (int i = 0; i < fullHearts; i++)
		{
			glm::vec2 heartPos = {firstHealthPos.x + i * (healthSize.x - magicOffsetX), firstHealthPos.y};
			m_HeartFull_Sprite.SetPosition(heartPos);
			m_HeartFull_Sprite.Render();
		}

		// Half Heart
		bool hasHalfHeart = (halfUnits % 2) != 0;
		if (hasHalfHeart)
		{
			m_HeartHalf_Sprite.SetSize(healthSize);
			glm::vec2 heartPos = {firstHealthPos.x + fullHearts * (healthSize.x - magicOffsetX), firstHealthPos.y};
			m_HeartHalf_Sprite.SetPosition(heartPos);
			m_HeartHalf_Sprite.Render();
		}

		// ---- Hunger (icons, bottom-right above hotbar) ----
		float hunger = player->GetHunger().CurrentHunger;
		float hungerSizeX = (36.f / 1920.f) * s_ScreenWidth;
		float hungerSizeY = (36.f / 1009.f) * s_ScreenHeight;
		glm::vec2 hungerSize = {hungerSizeX, hungerSizeY};
		float firstHungerRatioX = 1306.f / 1920.f;
		float firstHungerRatioY = firstHealthRatioY;
		glm::vec2 firstHungerPos = {s_ScreenWidth * firstHungerRatioX, s_ScreenHeight * firstHungerRatioY};

		// Hunger Containers
		m_HungerEmpty_Sprite.SetSize(hungerSize);
		magicOffsetX = (1.f / 9.f) * hungerSizeX;
		for (int i = 0; i < 10; i++)
		{
			glm::vec2 hungerPos = {firstHungerPos.x - i * (hungerSize.x - magicOffsetX), firstHungerPos.y};
			m_HungerEmpty_Sprite.SetPosition(hungerPos);
			m_HungerEmpty_Sprite.Render();
		}

		// Half Foods
		halfUnits = static_cast<int>(std::floor(hunger)); // number of half-foods
		if (hunger > 0.f && halfUnits == 0)
		{
			halfUnits = 1; // Ensure at least a half-food is shown for any positive hunger
		}
		int fullFoods = halfUnits / 2;
		m_HungerFull_Sprite.SetSize(hungerSize);
		for (int i = 0; i < fullFoods; i++)
		{
			glm::vec2 foodPos = {firstHungerPos.x - i * (hungerSize.x - magicOffsetX), firstHungerPos.y};
			m_HungerFull_Sprite.SetPosition(foodPos);
			m_HungerFull_Sprite.Render();
		}

		// Half Food
		bool hasHalfFood = (halfUnits % 2) != 0;
		if (hasHalfFood)
		{
			m_HungerHalf_Sprite.SetSize(hungerSize);
			glm::vec2 foodPos = {firstHungerPos.x - fullFoods * (hungerSize.x - magicOffsetX), firstHungerPos.y};
			m_HungerHalf_Sprite.SetPosition(foodPos);
			m_HungerHalf_Sprite.Render();
		}

		// ---- Experience Bar (bottom-center, between hotbar and hearts row) ----
		Experience::LevelInfo levelInfo = player->GetExperience().GetLevel();
		float xpProgress = levelInfo.ExperienceForNextLevel > 0
			? static_cast<float>(levelInfo.TotalExperienceForCurrentLevel) /
				static_cast<float>(levelInfo.ExperienceForNextLevel)
			: 0.f;

		float xpBarWidthRatio = 728.f / 1920.f;
		float xpBarHeightRatio = 19.f / 1009.f;
		glm::vec2 xpBarSize = {s_ScreenWidth * xpBarWidthRatio, s_ScreenHeight * xpBarHeightRatio};
		float xpBarCenterRatioY = (929 - 23) / 1009.f;
		glm::vec2 xpBarPos = {screenCenterX, s_ScreenHeight * xpBarCenterRatioY};

		glm::ivec2 xpBarCissorsTopLeft = {xpBarPos.x - (xpBarSize.x / 2), xpBarPos.y - (xpBarSize.y / 2)};
		float progressWidth = xpBarSize.x * xpProgress;
		glm::ivec2 xpBarCissorsBottomRight = {xpBarCissorsTopLeft.x + static_cast<int>(progressWidth),
											  xpBarPos.y + static_cast<int>(xpBarSize.y / 2)};

		float levelLabelCenterRatioX = 958.f / 1920.f;
		float levelLabelCenterRatioY = (909 - 23) / 1009.f;
		glm::vec2 levelLabelPos = {s_ScreenWidth * levelLabelCenterRatioX, s_ScreenHeight * levelLabelCenterRatioY};
		glm::vec3 labelColor = {1.f, 1.f, 1.f};

		m_ExperienceBarBackground_Sprite.SetPosition(xpBarPos);
		m_ExperienceBarBackground_Sprite.SetSize(xpBarSize);
		m_ExperienceBarBackground_Sprite.Render();

		m_ExperienceBarProgress_Sprite.SetPosition(xpBarPos);
		m_ExperienceBarProgress_Sprite.SetSize(xpBarSize);
		m_ExperienceBarProgress_Sprite.SetCissors(xpBarCissorsTopLeft, xpBarCissorsBottomRight);
		m_ExperienceBarProgress_Sprite.Render();

		if (levelInfo.Level > 0)
		{
			m_ExperienceLevel_Label.SetPosition(levelLabelPos);
			m_ExperienceLevel_Label.SetText(std::to_string(levelInfo.Level));
			m_ExperienceLevel_Label.SetTextHeight(s_TextHeight);
			m_ExperienceLevel_Label.Render();
		}

		// ---- Crosshair ----
		float crossHairHeightRatio = 35.f / 1009.f;
		float crossHairHeight = s_ScreenHeight * crossHairHeightRatio;
		float crossHairWidthRatio = 35.f / 1920.f;
		float crossHairWidth = s_ScreenWidth * crossHairWidthRatio;

		glm::vec2 centerScreenPos = {screenCenterX, s_ScreenHeight * 0.5f};
		glm::vec2 crosshairTop = {centerScreenPos.x, centerScreenPos.y - (crossHairHeight / 2)};
		glm::vec2 crosshairBottom = {centerScreenPos.x, centerScreenPos.y + (crossHairHeight / 2)};
		glm::vec2 crosshairLeft = {centerScreenPos.x - (crossHairWidth / 2), centerScreenPos.y};
		glm::vec2 crosshairRight = {centerScreenPos.x + (crossHairWidth / 2), centerScreenPos.y};

		// Draw crosshair as 2 lines
		glm::vec4 crosshairColor = {1.f, 1.f, 1.f, 1.f};
		int crosshairLineWidth = static_cast<int>(s_ScreenWidth * (4.f / 1920.f));
		DebugDraws::DrawScreenLine_Pixels(crosshairTop, crosshairBottom, crosshairColor, crosshairLineWidth);
		DebugDraws::DrawScreenLine_Pixels(crosshairLeft, crosshairRight, crosshairColor, crosshairLineWidth);
	}

	void HudPanel::Initialize()
	{
		m_Hotbar_Sprite.Initialize();
		m_HotbarSelection_Sprite.Initialize();
		m_HeartFull_Sprite.Initialize();
		m_HeartContainer_Sprite.Initialize();
		m_HeartHalf_Sprite.Initialize();
		m_HungerFull_Sprite.Initialize();
		m_HungerHalf_Sprite.Initialize();
		m_HungerEmpty_Sprite.Initialize();
		m_ExperienceBarBackground_Sprite.Initialize();
		m_ExperienceBarProgress_Sprite.Initialize();
		m_ExperienceLevel_Label.Initialize();

		SetInitState(true);
	}

	void HudPanel::Delete()
	{
		m_Hotbar_Sprite.Delete();
		m_HotbarSelection_Sprite.Delete();
		m_HeartFull_Sprite.Delete();
		m_HeartContainer_Sprite.Delete();
		m_HeartHalf_Sprite.Delete();
		m_HungerFull_Sprite.Delete();
		m_HungerHalf_Sprite.Delete();
		m_HungerEmpty_Sprite.Delete();
		m_ExperienceBarBackground_Sprite.Delete();
		m_ExperienceBarProgress_Sprite.Delete();
		m_ExperienceLevel_Label.Delete();

		SetDeletedState(true);
	}

	void HudPanel::ReloadTextures()
	{
		m_Hotbar_Sprite.ReloadTextures();
		m_HotbarSelection_Sprite.ReloadTextures();
		m_HeartFull_Sprite.ReloadTextures();
		m_HeartContainer_Sprite.ReloadTextures();
		m_HeartHalf_Sprite.ReloadTextures();
		m_HungerFull_Sprite.ReloadTextures();
		m_HungerHalf_Sprite.ReloadTextures();
		m_HungerEmpty_Sprite.ReloadTextures();
		m_ExperienceBarBackground_Sprite.ReloadTextures();
		m_ExperienceBarProgress_Sprite.ReloadTextures();
		m_ExperienceLevel_Label.ReloadTextures();
	}

} // namespace onion::voxel
