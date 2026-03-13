#pragma once

#include <renderer/gui/GuiElement.hpp>
#include <renderer/gui/controls/button/Button.hpp>
#include <renderer/gui/controls/label/Label.hpp>
#include <renderer/gui/controls/sprite/Sprite.hpp>
#include <renderer/utils/SinusPulseGenerator.hpp>

namespace onion::voxel
{
	class MainMenuPanel : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		MainMenuPanel(const std::string& name);
		~MainMenuPanel() override;

		// ----- Public API -----
	  public:
		void Render() override;
		void Initialize() override;
		void Delete() override;
		void ReloadTextures() override;

		void CycleSplashText();

		// ----- Public Events -----
	  public:
		Event<const std::pair<const GuiElement*, eMenu>&> RequestMenuNavigation;
		Event<const GuiElement*> RequestQuitGame;

		// ----- Properties -----
	  private:
		static inline const std::filesystem::path s_SpriteTitlePath =
			std::filesystem::path("textures") / "OnionVoxelTitle2.png";
		static inline const std::filesystem::path s_SplashScreenTextPath =
			std::filesystem::path("minecraft") / "texts" / "splashes.txt";

		// ----- Controls -----
	  private:
		Sprite m_Title_Sprite;
		Button m_Singleplayer_Button;
		Button m_Multiplayer_Button;
		Button m_DemoPanel_Button;
		Button m_Options_Button;
		Button m_QuitGame_Button;
		Label m_SplashText_Label;
		Label m_Version_Label;
		Label m_Copyright_Label;

		// ----- Internal Values -----
	  private:
		void LoadSplashes();
		std::vector<std::string> m_Splashes;
		int m_CurrentSplashIndex = 0;
		SinusPulseGenerator m_SplashTextPulse{1, 0.1f, 1.f};

		// ----- Internal Event Subscription and Handlers -----
	  private:
		std::vector<EventHandle> m_EventHandles;
		void SubscribeToControlEvents();

		void Handle_Singleplayer_Click(const Button& sender);
		void Handle_Multiplayer_Click(const Button& sender);
		void Handle_DemoButton_Click(const Button& sender);
		void Handle_Options_Click(const Button& sender);
		void Handle_QuitGame_Click(const Button& sender);
	};
} // namespace onion::voxel
