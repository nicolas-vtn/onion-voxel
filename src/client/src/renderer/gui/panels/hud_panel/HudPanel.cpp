#include "HudPanel.hpp"

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
		  m_HungerEmpty_Sprite("HungerEmpty_Sprite", s_PathHungerEmpty, Sprite::eOrigin::ResourcePack)
	{
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
		float spacingRatio = hotbarWidthRatio / 9.f;
		float leftHotbarEdgeRatio = 0.5f - hotbarWidthRatio * 0.5f;
		int selectedSlotIndex = player->GetHotbar().SelectedSlot;
		float selectionXratio = leftHotbarEdgeRatio + (spacingRatio / 2.f) + (spacingRatio * selectedSlotIndex);
		glm::vec2 hotbarSelectionPos = {s_ScreenWidth * selectionXratio, screenBottom - hotbarHeight / 2};
		m_HotbarSelection_Sprite.SetPosition(hotbarSelectionPos);
		m_HotbarSelection_Sprite.SetSize({slotSizeX, slotSizeY});
		m_HotbarSelection_Sprite.Render();

		// ---- Health (hearts, bottom-left above hotbar) ----
		float health = player->GetHealth().CurrentHealth;
		float healthSizeX = (36.f / 1920.f) * s_ScreenWidth;
		float healthSizeY = (36.f / 1009.f) * s_ScreenHeight;
		glm::vec2 healthSize = {healthSizeX, healthSizeY};
		float firstHealthRatioX = 614.f / 1920.f;
		float firstHealthRatioY = 872.f / 1009.f;
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
		float firstHungerRatioX = 1305.f / 1920.f;
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
		//// ---- Experience Bar (bottom-center, between hotbar and hearts row) ----
		//glm::vec2 xpBarSize = {hotbarSize.x, s_ScreenHeight * 0.009f};
		//glm::vec2 xpBarPos = {screenCenterX,
		//					  screenBottom - hotbarSize.y - iconSize - xpBarSize.y * 0.5f - s_ScreenHeight * 0.01f};

		//m_ExperienceBarBackground_Sprite.SetPosition(xpBarPos);
		//m_ExperienceBarBackground_Sprite.SetSize(xpBarSize);
		//m_ExperienceBarBackground_Sprite.Render();

		//m_ExperienceBarProgress_Sprite.SetPosition(xpBarPos);
		//m_ExperienceBarProgress_Sprite.SetSize(xpBarSize);
		//m_ExperienceBarProgress_Sprite.Render();
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
	}

} // namespace onion::voxel
