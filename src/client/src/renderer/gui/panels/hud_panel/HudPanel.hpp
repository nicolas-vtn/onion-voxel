#pragma once

#include <renderer/gui/GuiElement.hpp>
#include <renderer/gui/controls/label/Label.hpp>
#include <renderer/gui/controls/sprite/Sprite.hpp>

#include <renderer/gui/ui_block_mesh/UiBlockMesh.hpp>

namespace onion::voxel
{
	class HudPanel : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		HudPanel(const std::string& name);
		~HudPanel() override = default;

		// ----- Public API -----
	  public:
		void Render() override;
		void Initialize() override;
		void Delete() override;
		void ReloadTextures() override;

		// ----- Controls -----
	  private:
		Sprite m_Hotbar_Sprite;
		Sprite m_HotbarSelection_Sprite;
		Sprite m_HeartContainer_Sprite;
		Sprite m_HeartFull_Sprite;
		Sprite m_HeartHalf_Sprite;
		Sprite m_HungerEmpty_Sprite;
		Sprite m_HungerFull_Sprite;
		Sprite m_HungerHalf_Sprite;
		Sprite m_ExperienceBarBackground_Sprite;
		Sprite m_ExperienceBarProgress_Sprite;
		Label m_ExperienceLevel_Label;
		Label m_SelectedBlockName_Label;

		std::shared_ptr<UiBlockMesh> m_UiBlockMesh = std::make_shared<UiBlockMesh>(Inventory{1, 9});

		// ----- State -----
	  private:
		int m_PreviousSelectedHotbarIndex = -1; // The previously selected hotbar index
		double m_SelectedBlockTime = 0.0;		// Time in seconds that the currently selected block has been selected.

		// ----- Private Helpers -----
	  private:
		float GetSelectedBlockNameFadeInFactor() const;

		// ----- Textures -----
	  private:
		static inline const std::filesystem::path s_PathHotbar =
			GuiElement::s_BasePathGuiAssets / "sprites" / "hud" / "hotbar.png";

		static inline const std::filesystem::path s_PathHotbarSelection =
			GuiElement::s_BasePathGuiAssets / "sprites" / "hud" / "hotbar_selection.png";

		static inline const std::filesystem::path s_PathHeartFull =
			GuiElement::s_BasePathGuiAssets / "sprites" / "hud" / "heart" / "full.png";

		static inline const std::filesystem::path s_PathHeartHalf =
			GuiElement::s_BasePathGuiAssets / "sprites" / "hud" / "heart" / "half.png";

		static inline const std::filesystem::path s_PathHeartContainer =
			GuiElement::s_BasePathGuiAssets / "sprites" / "hud" / "heart" / "container.png";

		static inline const std::filesystem::path s_PathHungerEmpty =
			GuiElement::s_BasePathGuiAssets / "sprites" / "hud" / "food_empty.png";

		static inline const std::filesystem::path s_PathHungerFull =
			GuiElement::s_BasePathGuiAssets / "sprites" / "hud" / "food_full.png";

		static inline const std::filesystem::path s_PathHungerHalf =
			GuiElement::s_BasePathGuiAssets / "sprites" / "hud" / "food_half.png";

		static inline const std::filesystem::path s_PathExperienceBarBackground =
			GuiElement::s_BasePathGuiAssets / "sprites" / "hud" / "experience_bar_background.png";

		static inline const std::filesystem::path s_PathExperienceBarProgress =
			GuiElement::s_BasePathGuiAssets / "sprites" / "hud" / "experience_bar_progress.png";
	};
} // namespace onion::voxel
