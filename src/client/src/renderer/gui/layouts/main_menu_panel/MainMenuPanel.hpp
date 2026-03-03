#pragma once

#include "../../GuiElement.hpp"
#include "../../controls/button/Button.hpp"
#include "../../controls/sprite/Sprite.hpp"

namespace onion::voxel
{
	class MainMenuPanel : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		MainMenuPanel(const std::string& name);
		~MainMenuPanel() override = default;

		// ----- Public API -----
	  public:
		void Render() override;
		void Initialize() override;
		void Delete() override;

		void SetGameVersion(const std::string& version);

		// ----- Public Events -----
	  public:
		Event<const eMenu&> RequestMenuNavigation;
		Event<const GuiElement*> RequestQuitGame;

		// ----- Properties -----
	  private:
		std::filesystem::path m_SpriteTitlePath = GetAssetsPath() / "textures" / "OnionVoxelTitle2.png";

		std::string m_GameVersion = "0.1.0";

		// ----- Controls -----
	  private:
		Sprite m_Title_Sprite;
		Button m_Singleplayer_Button;
		Button m_Multiplayer_Button;
		Button m_DemoPanel_Button;
		Button m_Options_Button;
		Button m_QuitGame_Button;

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
