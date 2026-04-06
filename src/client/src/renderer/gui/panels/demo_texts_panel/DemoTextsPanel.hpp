#pragma once

#include <renderer/gui/GuiElement.hpp>
#include <renderer/gui/controls/button/Button.hpp>
#include <renderer/gui/controls/checkbox/Checkbox.hpp>
#include <renderer/gui/controls/slider/Slider.hpp>
#include <renderer/gui/controls/sprite/Sprite.hpp>
#include <renderer/gui/controls/text_field/TextField.hpp>

namespace onion::voxel
{
	class DemoTextsPanel : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		DemoTextsPanel(const std::string& name);
		~DemoTextsPanel() override;

		// ----- Public API -----
	  public:
		void Render() override;
		void Initialize() override;
		void Delete() override;
		void ReloadTextures() override;

		// ----- Public Events -----
	  public:
		Event<const GuiElement*> EvtRequestBackNavigation;

		// ----- Properties -----
	  private:
		std::string m_AllTextStylesLabelText_1 =
			"§0Black §1DarkBlue §2DarkGreen §3DarkAqua §4DarkRed §5DarkPurple §6Gold §7Gray §8DarkGray §9Blue";
		std::string m_AllTextStylesLabelText_2 =
			"§aGreen §bAqua §cRed §dLightPurple §eYellow §fWhite §lBold §mStrikethrough §nUnderline §oItalic";

		// ----- Controls -----
	  private:
		std::vector<std::unique_ptr<Label>> m_Labels;
		Label m_LabelAllStyles_1;
		Label m_LabelAllStyles_2;

		Button m_BackButton;

		// ----- Internal Event Subscription and Handlers -----
	  private:
		void SubscribeToControlEvents();

		std::vector<EventHandle> m_EventHandles;

		void Handle_BackButtonClick(const Button& button);

		// ----- Enums -----
	  private:
		enum class LabelType
		{
			Regular,

			Color0,
			Color1,
			Color2,
			Color3,
			Color4,
			Color5,
			Color6,
			Color7,
			Color8,
			Color9,
			ColorA,
			ColorB,
			ColorC,
			ColorD,
			ColorE,
			ColorF,

			Bold,
			Strikethrough,
			Underline,
			Italic,

			Count
		};
	};
} // namespace onion::voxel
