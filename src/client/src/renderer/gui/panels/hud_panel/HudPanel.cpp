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
			  "ExperienceBarProgress_Sprite", s_PathExperienceBarProgress, Sprite::eOrigin::ResourcePack)
	{
	}

	void HudPanel::Render()
	{
		// ---- Constants ----
		float screenCenterX = s_ScreenWidth * 0.5f;
		float screenBottom = s_ScreenHeight;

		// ---- Hotbar (bottom-center) ----
		glm::vec2 hotbarSize = {s_ScreenWidth * 0.364f, s_ScreenHeight * 0.0556f};
		glm::vec2 hotbarPos = {screenCenterX, screenBottom - hotbarSize.y * 0.5f};
		m_Hotbar_Sprite.SetPosition(hotbarPos);
		m_Hotbar_Sprite.SetSize(hotbarSize);
		m_Hotbar_Sprite.Render();

		// ---- Hotbar Selection (first slot, overlaid on hotbar) ----
		float slotSize = hotbarSize.y;
		glm::vec2 hotbarSelectionPos = {screenCenterX - hotbarSize.x * 0.5f + slotSize * 0.5f, hotbarPos.y};
		m_HotbarSelection_Sprite.SetPosition(hotbarSelectionPos);
		m_HotbarSelection_Sprite.SetSize({slotSize, slotSize});
		m_HotbarSelection_Sprite.Render();

		// ---- Health (hearts, bottom-left above hotbar) ----
		float iconSize = s_ScreenHeight * 0.028f;
		float hudRowY = screenBottom - hotbarSize.y - iconSize * 0.5f - s_ScreenHeight * 0.005f;
		float heartStartX = screenCenterX - hotbarSize.x * 0.5f;

		m_HeartFull_Sprite.SetPosition({heartStartX, hudRowY});
		m_HeartFull_Sprite.SetSize({iconSize, iconSize});
		m_HeartFull_Sprite.Render();

		m_HeartHalf_Sprite.SetPosition({heartStartX + iconSize, hudRowY});
		m_HeartHalf_Sprite.SetSize({iconSize, iconSize});
		m_HeartHalf_Sprite.Render();

		// ---- Hunger (icons, bottom-right above hotbar) ----
		float hungerStartX = screenCenterX + hotbarSize.x * 0.5f - iconSize;

		m_HungerFull_Sprite.SetPosition({hungerStartX, hudRowY});
		m_HungerFull_Sprite.SetSize({iconSize, iconSize});
		m_HungerFull_Sprite.Render();

		m_HungerHalf_Sprite.SetPosition({hungerStartX - iconSize, hudRowY});
		m_HungerHalf_Sprite.SetSize({iconSize, iconSize});
		m_HungerHalf_Sprite.Render();

		// ---- Experience Bar (bottom-center, between hotbar and hearts row) ----
		glm::vec2 xpBarSize = {hotbarSize.x, s_ScreenHeight * 0.009f};
		glm::vec2 xpBarPos = {screenCenterX,
							  screenBottom - hotbarSize.y - iconSize - xpBarSize.y * 0.5f - s_ScreenHeight * 0.01f};

		m_ExperienceBarBackground_Sprite.SetPosition(xpBarPos);
		m_ExperienceBarBackground_Sprite.SetSize(xpBarSize);
		m_ExperienceBarBackground_Sprite.Render();

		m_ExperienceBarProgress_Sprite.SetPosition(xpBarPos);
		m_ExperienceBarProgress_Sprite.SetSize(xpBarSize);
		m_ExperienceBarProgress_Sprite.Render();
	}

	void HudPanel::Initialize()
	{
		m_Hotbar_Sprite.Initialize();
		m_HotbarSelection_Sprite.Initialize();
		m_HeartFull_Sprite.Initialize();
		m_HeartHalf_Sprite.Initialize();
		m_HungerFull_Sprite.Initialize();
		m_HungerHalf_Sprite.Initialize();
		m_ExperienceBarBackground_Sprite.Initialize();
		m_ExperienceBarProgress_Sprite.Initialize();

		SetInitState(true);
	}

	void HudPanel::Delete()
	{
		m_Hotbar_Sprite.Delete();
		m_HotbarSelection_Sprite.Delete();
		m_HeartFull_Sprite.Delete();
		m_HeartHalf_Sprite.Delete();
		m_HungerFull_Sprite.Delete();
		m_HungerHalf_Sprite.Delete();
		m_ExperienceBarBackground_Sprite.Delete();
		m_ExperienceBarProgress_Sprite.Delete();

		SetDeletedState(true);
	}

	void HudPanel::ReloadTextures()
	{
		m_Hotbar_Sprite.ReloadTextures();
		m_HotbarSelection_Sprite.ReloadTextures();
		m_HeartFull_Sprite.ReloadTextures();
		m_HeartHalf_Sprite.ReloadTextures();
		m_HungerFull_Sprite.ReloadTextures();
		m_HungerHalf_Sprite.ReloadTextures();
		m_ExperienceBarBackground_Sprite.ReloadTextures();
		m_ExperienceBarProgress_Sprite.ReloadTextures();
	}

} // namespace onion::voxel
