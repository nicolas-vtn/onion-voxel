#pragma once

#include "../../GuiElement.hpp"
#include "../../controls/button/Button.hpp"
#include "../../controls/sprite/Sprite.hpp"

namespace onion::voxel
{
	class DemoPanel : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		DemoPanel(const std::string& name);
		~DemoPanel() override = default;

		// ----- Public API -----
	  public:
		void Render() override;
		void Initialize() override;
		void Delete() override;

		// ----- Public Events -----
	  public:
		Event<const eMenu&> RequestMenuNavigation;

		// ----- Properties -----
	  private:
		std::string m_SpritePath = (GetAssetsPath() / "textures" / "OnionVoxelTitle.png").string();

		// ----- Controls -----
	  private:
		Button m_Button;
		Button m_Button2;
		Button m_ButtonMainMenu;
		Sprite m_Sprite;

		// ----- Internal Event Subscription and Handlers -----
	  private:
		void SubscribeToControlEvents();

		EventHandle m_HandleButtonClick;
		void HandleButtonClick(const Button& button);

		EventHandle m_HandleButtonHoverEnter;
		void HandleButtonHoverEnter(const Button& button);

		EventHandle m_HandleButtonHoverLeave;
		void HandleButtonHoverLeave(const Button& button);

		EventHandle m_HandleButtonMainMenuClick;
		void HandleButtonMainMenuClick(const Button& button);
	};
} // namespace onion::voxel
