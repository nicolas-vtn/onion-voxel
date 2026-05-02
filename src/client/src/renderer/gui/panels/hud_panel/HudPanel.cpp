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
		  m_ExperienceLevel_Label("ExperienceLevel_Label"), m_SelectedBlockName_Label("SelectedBlockName_Label"),
		  m_Fps_Label("Fps_Label"), m_WailaTooltip("WailaTooltip")
	{
		m_ExperienceBarBackground_Sprite.SetZOffset(0.4f);
		m_ExperienceBarProgress_Sprite.SetZOffset(0.45f);

		m_ExperienceLevel_Label.SetZOffset(0.5f);
		m_ExperienceLevel_Label.SetTextColor(Font::eColor::Green);
		m_ExperienceLevel_Label.SetTextAlignment(Font::eTextAlignment::Center);

		m_SelectedBlockName_Label.SetZOffset(0.5f);
		m_SelectedBlockName_Label.SetTextAlignment(Font::eTextAlignment::Center);

		m_Fps_Label.SetZOffset(0.5f);
		m_Fps_Label.SetTextAlignment(Font::eTextAlignment::Left);

		m_UiBlockMesh->SetRenderSelectedHighlight(false);
		m_UiBlockMesh->SetSlotBorder(0.f);

		m_WailaTooltip.SetZOffset(0.85f);
		m_WailaBlockMesh->SetRenderSelectedHighlight(false);
		m_WailaBlockMesh->SetSlotBorder(0.f);
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

		Inventory playerHotbar = player->GetHotbar();

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
			const int maxSlots = playerHotbar.Rows() * playerHotbar.Columns();

			int current = static_cast<int>(playerHotbar.SelectedIndex());

			// Proper modulo wrapping (handles negative values correctly)
			int newSlot = (current - scroll) % maxSlots;
			if (newSlot < 0)
				newSlot += maxSlots;

			playerHotbar.SelectedIndex() = static_cast<uint8_t>(newSlot);
			player->SetHotbar(playerHotbar);
		}

		// Detect hotbar selection change to reset the selected block name timer
		bool selectionChanged = (playerHotbar.SelectedIndex() != m_PreviousSelectedHotbarIndex);
		if (selectionChanged)
		{
			m_SelectedBlockTime = glfwGetTime(); // Reset timer
		}

		// ---- Constants ----
		int screenCenterX = static_cast<int>(std::round(s_ScreenWidth * 0.5f));
		int screenBottom = static_cast<int>(s_ScreenHeight);

		// ---- FPS (top-left) ----
		{
			double now = glfwGetTime();
			double delta = now - m_LastFrameTime;
			m_LastFrameTime = now;
			if (delta > 0.0)
			{
				float instant = static_cast<float>(1.0 / delta);
				m_SmoothedFps += (instant - m_SmoothedFps) * 0.1f;
			}
			int fps = static_cast<int>(std::round(m_SmoothedFps));
			m_Fps_Label.SetText("Fps: " + std::to_string(fps));
			m_Fps_Label.SetTextHeight(s_TextHeight);
			m_Fps_Label.SetPosition({s_ScreenWidth * (10.f / 1920.f), s_ScreenHeight * (20.f / 1009.f)});
			m_Fps_Label.Render();
		}

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
		int selectedSlotIndex = playerHotbar.SelectedIndex();
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
		m_UiBlockMesh->SetInventory(playerHotbar, slotSize, slotPadding);
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

		// ---- Selected Block Name (top-center) ----
		float textFadeStrength = GetSelectedBlockNameFadeInFactor();
		if (textFadeStrength > 0.f)
		{
			BlockId selectedBlockId = playerHotbar.At(playerHotbar.SelectedIndex());
			if (selectedBlockId != BlockId::Air)
			{
				std::string blockName = BlockIds::GetName(playerHotbar.At(playerHotbar.SelectedIndex()));
				const float labelYposRatio = (812.f - 23.f) / 1009.f;
				const float labelPosY = std::round(s_ScreenHeight * labelYposRatio);
				m_SelectedBlockName_Label.SetText(blockName);
				m_SelectedBlockName_Label.SetPosition({screenCenterX, labelPosY});
				m_SelectedBlockName_Label.SetTextHeight(s_TextHeight);
				glm::vec4 textColor = glm::vec4(1.f, 1.f, 1.f, textFadeStrength);
				m_SelectedBlockName_Label.SetCustomTextColor(textColor);
				m_SelectedBlockName_Label.Render();
			}
		}

		// ---- WAILA (What Am I Looking At) ----
		if (EngineContext::Get().Settings().Video.WailaEnabled)
		{
			const auto& lookedAt = EngineContext::Get().LookedAtBlock;
			if (lookedAt.has_value() && lookedAt->HitBlock.State.ID != BlockId::Air)
			{
				const BlockId wailaBlockId = lookedAt->HitBlock.State.ID;

				// Build tooltip text — leading spaces reserve room for the block miniature on the left.
				// NOTE: spaces on line 2 must come AFTER the format codes (§9§o), not before them.
				// SegmentText splits on § boundaries, so any text before a § is flushed into the previous
				// segment. Spaces placed before §9§o would end up in segment 1 and their cursor advance
				// would be discarded when RenderText resets X to startX after the newline.
				static constexpr std::string_view k_WailaSpaces = "      "; // 6 spaces
				const std::string wailaLine1 = std::string(k_WailaSpaces) + BlockIds::GetName(wailaBlockId);
				const std::string wailaLine2 = "§9§o" + std::string(k_WailaSpaces) + "Onion::Voxel§r§r";

				m_WailaTooltip.SetText(wailaLine1 + "\n" + wailaLine2);
				m_WailaTooltip.SetTextHeight(s_TextHeight);

				// Position: top-center — m_Position.y is the vertical center of the tooltip box.
				const float wailaTooltipY = s_ScreenHeight * (60.f / 1009.f);
				m_WailaTooltip.SetPositionCentered({0.f, wailaTooltipY});

				// Compute where the block mesh should go (inner top-left of the tooltip content area).
				const glm::ivec2 innerTopLeft = m_WailaTooltip.GetInnerTopLeft();

				// Block mesh: square slot sized to span the full inner content height (two text lines + gap).
				const float blockSize = s_TextHeight * 2.6f;
				const glm::vec2 wailaSlotSize = {blockSize, blockSize};

				// Center the block mesh vertically within the content area.
				const float blockTopY = innerTopLeft.y + (s_TextHeight * 2.2f) / 2.f - blockSize / 2.f;
				const glm::vec2 blockTopLeft = {(float) innerTopLeft.x, blockTopY};

				// Update block mesh inventory.
				Inventory wailaInv{1, 1};
				wailaInv.Content()[0] = wailaBlockId;
				m_WailaBlockMesh->SetInventory(wailaInv, wailaSlotSize, {0.f, 0.f});
				if (m_WailaBlockMesh->IsDirty())
				{
					auto& meshBuilder = EngineContext::Get().WrldRenderer->GetMeshBuilder();
					meshBuilder.UpdateUiBlockMesh(m_WailaBlockMesh);
				}

				m_WailaTooltip.Render();
				m_WailaBlockMesh->Render(blockTopLeft, s_ScreenWidth, s_ScreenHeight);
			}
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

		// Update States
		m_PreviousSelectedHotbarIndex = playerHotbar.SelectedIndex();
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
		m_Fps_Label.Initialize();
		m_WailaTooltip.Initialize();

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
		m_Fps_Label.Delete();
		m_UiBlockMesh->Delete();
		m_WailaTooltip.Delete();
		m_WailaBlockMesh->Delete();

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
		m_Fps_Label.ReloadTextures();
		m_WailaTooltip.ReloadTextures();
		m_WailaBlockMesh->SetDirty(true);
	}

	float HudPanel::GetSelectedBlockNameFadeInFactor() const
	{
		double t = glfwGetTime() - m_SelectedBlockTime;

		if (t <= 2.0)
			return 1.0f;

		if (t >= 3.0)
			return 0.0f;

		// Linear fade from 1 to 0 over [2, 3]
		return static_cast<float>(1.0 - (t - 2.0));
	}

} // namespace onion::voxel
