#pragma once

#include "../../GuiElement.hpp"
#include "../../controls/button/Button.hpp"
#include "../../controls/checkbox/Checkbox.hpp"
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
		Event<const std::pair<const GuiElement*, eMenu>&> RequestMenuNavigation;

		// ----- Properties -----
	  private:
		std::string m_SpritePath = (GetAssetsPath() / "textures" / "OnionVoxelTitle.png").string();

		// ----- Controls -----
	  private:
		Button m_Button;
		Button m_Button2;
		Button m_ButtonMainMenu;
		Sprite m_Sprite;
		Checkbox m_Checkbox;

		// ----- Internal Event Subscription and Handlers -----
	  private:
		void SubscribeToControlEvents();

		EventHandle m_HandleButtonClick;
		void Handle_ButtonClick(const Button& button);

		EventHandle m_HandleButtonHoverEnter;
		void Handle_ButtonHoverEnter(const Button& button);

		EventHandle m_HandleButtonHoverLeave;
		void Handle_ButtonHoverLeave(const Button& button);

		EventHandle m_HandleButtonMainMenuClick;
		void Handle_ButtonMainMenuClick(const Button& button);

		EventHandle m_HandleCheckboxCheckedChanged;
		void Handle_CheckboxCheckedChanged(const Checkbox& checkbox);
	};
} // namespace onion::voxel
