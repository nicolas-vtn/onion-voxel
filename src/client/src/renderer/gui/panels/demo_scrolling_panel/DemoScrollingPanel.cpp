#include "DemoScrollingPanel.hpp"

namespace onion::voxel
{
	DemoScrollingPanel::DemoScrollingPanel(const std::string& name)
		: GuiElement(name), m_ButtonBack("DemoScrollingPanelBackButton"),
		  m_SliderButtonCount("DemoScrollingPanelSlider"), m_Scroller("DemoScrollingPanelScroller"),
		  m_SliderDemo("DemoScrollingPanelSliderDemo"), m_LabelDemo("DemoScrollingPanelLabelDemo"),
		  m_SpriteDemo("DemoScrollingPanelSpriteDemo", m_SpritePath, Sprite::eOrigin::Asset),
		  m_CheckboxDemo("DemoScrollingPanelCheckboxDemo"), m_TextFieldDemo("DemoScrollingPanelTextFieldDemo")
	{
		SubscribeToControlEvents();

		m_ButtonBack.SetPosition({400, 500});
		m_ButtonBack.SetSize({200.f, 40.f});
		m_ButtonBack.SetText("Back");
		m_ButtonBack.SetEnabled(true);

		m_SliderButtonCount.SetPosition({400, 600});
		m_SliderButtonCount.SetSize({200.f, 40.f});
		m_SliderButtonCount.SetValue(0);
		m_SliderButtonCount.SetMaxValue(100);
		m_SliderButtonCount.SetValue(20);

		m_Scroller.SetTopLeftCorner({100, 100});
		m_Scroller.SetBottomRightCorner({500, 500});
		m_Scroller.SetScrollAreaHeight(400);
		m_Scroller.SetScrollRatio(0.0f);

		m_SliderDemo.SetPosition({400, 700});
		m_SliderDemo.SetSize({200.f, 40.f});
		m_SliderDemo.SetValue(0);
		m_SliderDemo.SetMaxValue(100);

		m_LabelDemo.SetPosition({400, 800});
		m_LabelDemo.SetTextAlignment(Font::eTextAlignment::Center);
		m_LabelDemo.SetText("Slider Value: 0");

		m_SpriteDemo.SetPosition({400, 900});
		m_SpriteDemo.SetSize({200.f, 100.f});

		m_CheckboxDemo.SetPosition({400, 1000});
		m_CheckboxDemo.SetSize({40.f, 40.f});
		m_CheckboxDemo.SetChecked(false);

		m_TextFieldDemo.SetPosition({400, 1100});
		m_TextFieldDemo.SetSize({200.f, 40.f});
		m_TextFieldDemo.SetText("");
		m_TextFieldDemo.SetPlaceholderText("Type something...");
	}

	void DemoScrollingPanel::Render()
	{
		// ---- Calculate Control Sizes and Positions ----
		glm::vec2 controlsSizeRatio{0.415f, 0.08f};
		glm::vec2 controlsSize{controlsSizeRatio.x * s_ScreenWidth, controlsSizeRatio.y * s_ScreenHeight};
		int centerX = s_ScreenWidth / 2;

		// ---- Constants for Dynamic Buttons Layout ----
		float dynamicButtonsStartYRatio = 0.4f;
		float dynamicButtonsSpacingYRatio = 0.1f;

		// ---- Pre Configure ----
		// Add or remove dynamic buttons based on the slider value
		AdjustDynamicButtons(m_SliderButtonCount.GetValue());

		// ---- Render Slider ----
		float sliderPositionRatioY = 0.1f;
		m_SliderButtonCount.SetPosition({centerX, s_ScreenHeight * sliderPositionRatioY});
		m_SliderButtonCount.SetSize(controlsSize);
		m_SliderButtonCount.SetText("Button Count: " + std::to_string(m_SliderButtonCount.GetValue()));

		m_SliderButtonCount.Render();

		// ---- Render Scroller ----
		float scrollerWidthRatio = 0.5f;
		float scrollerHeightRatio = 0.6f;
		glm::ivec2 scrollerSize{static_cast<int>(s_ScreenWidth * scrollerWidthRatio),
								static_cast<int>(s_ScreenHeight * scrollerHeightRatio)};
		glm::ivec2 scrollCenter{centerX, s_ScreenHeight / 2};

		glm::ivec2 scrollerTopLeftCorner{scrollCenter.x - scrollerSize.x / 2, scrollCenter.y - scrollerSize.y / 2};
		glm::ivec2 scrollerBottomRightCorner{scrollCenter.x + scrollerSize.x / 2, scrollCenter.y + scrollerSize.y / 2};

		// Calculate the height of the scroll area based on the number of dynamic buttons and their spacing
		float totalSspacingYButtonsRatio = m_DynamicButtons.size() * dynamicButtonsSpacingYRatio;
		int scrollAreaHeight = static_cast<int>(s_ScreenHeight * totalSspacingYButtonsRatio);

		m_Scroller.SetScrollAreaHeight(scrollAreaHeight);
		m_Scroller.SetTopLeftCorner(scrollerTopLeftCorner);
		m_Scroller.SetBottomRightCorner(scrollerBottomRightCorner);

		m_Scroller.Render();

		// ---- Start Cissoring for Scroller ----
		m_Scroller.StartCissoring();

		// ---- Render Demo Label ----
		float labelPositionRatioY = 0.2f;
		m_LabelDemo.SetPosition({centerX, s_ScreenHeight * labelPositionRatioY});

		// ---- Render Dynamic Buttons ----
		for (size_t i = 0; i < m_DynamicButtons.size(); i++)
		{
			auto& button = m_DynamicButtons[i];

			float buttonYRatio = dynamicButtonsStartYRatio + i * dynamicButtonsSpacingYRatio;
			glm::ivec2 buttonPosition{centerX, s_ScreenHeight * buttonYRatio};
			buttonPosition = m_Scroller.ProjectContentPosition(buttonPosition);

			Visibility visibility = m_Scroller.GetControlVisibleArea(buttonPosition, controlsSize);

			button->SetPosition(buttonPosition);
			button->SetSize(controlsSize);
			button->SetVisibility(visibility);

			//if (i == 6)
			//{
			//	std::cout << "Button " << i + 1 << " Visibility: " << (visibility.IsVisible ? "Visible" : "Not Visible")
			//			  << ", Fully Visible: " << (visibility.IsFullyVisible ? "Yes" : "No") << std::endl;
			//	std::cout << "Visible Area Top Left: (" << visibility.VisibleAreaTopLeftCorner.x << ", "
			//			  << visibility.VisibleAreaTopLeftCorner.y << "), Bottom Right: ("
			//			  << visibility.VisibleAreaBottomRightCorner.x << ", "
			//			  << visibility.VisibleAreaBottomRightCorner.y << ")" << std::endl;
			//}

			if (visibility.IsVisible)
			{
				button->Render();
			}
		}

		// ---- Stop Cissoring for Scroller ----
		m_Scroller.StopCissoring();

		// ---- Render Back Button ----
		float positionRatioY = 0.9f;

		m_ButtonBack.SetPosition({centerX, s_ScreenHeight * positionRatioY});
		m_ButtonBack.SetSize(controlsSize);
		m_ButtonBack.Render();
	}

	void DemoScrollingPanel::Initialize()
	{
		m_ButtonBack.Initialize();
		m_SliderButtonCount.Initialize();
		m_Scroller.Initialize();
		for (auto& button : m_DynamicButtons)
		{
			button->Initialize();
		}

		SetInitState(true);
	}

	void DemoScrollingPanel::Delete()
	{
		m_ButtonBack.Delete();
		m_SliderButtonCount.Delete();
		m_Scroller.Delete();
		for (auto& button : m_DynamicButtons)
		{
			button->Delete();
		}

		SetDeletedState(true);
	}

	void DemoScrollingPanel::ReloadTextures()
	{
		m_ButtonBack.ReloadTextures();
		m_SliderButtonCount.ReloadTextures();
		m_Scroller.ReloadTextures();
		for (auto& button : m_DynamicButtons)
		{
			button->ReloadTextures();
		}
	}

	void DemoScrollingPanel::AdjustDynamicButtons(size_t buttonCount)
	{
		// Remove excess buttons if buttonCount is less than the current number of buttons
		while (m_DynamicButtons.size() > buttonCount)
		{
			Button& buttonToRemove = *m_DynamicButtons.back();
			buttonToRemove.Delete();
			m_DynamicButtons.pop_back();
		}

		// Add new buttons if buttonCount is greater than the current number of buttons
		while (m_DynamicButtons.size() < buttonCount)
		{
			size_t newButtonIndex = m_DynamicButtons.size() + 1;
			auto newButton = std::make_unique<Button>("DynamicButton" + std::to_string(newButtonIndex));
			newButton->Initialize();
			newButton->SetText("Button " + std::to_string(newButtonIndex));
			m_DynamicButtons.push_back(std::move(newButton));
		}
	}

	void DemoScrollingPanel::SubscribeToControlEvents()
	{
		m_EventHandles.push_back(
			m_ButtonBack.OnClick.Subscribe([this](const Button& button) { Handle_ButtonBackClick(button); }));

		m_EventHandles.push_back(m_SliderButtonCount.OnValueChanged.Subscribe(
			[this](const Slider& slider) { Handle_SliderButtonCountValueChanged(slider); }));

		m_EventHandles.push_back(m_Scroller.OnScrollRatioChanged.Subscribe(
			[this](const Scroller& scroller) { Handle_ScrollerScrollRatioChanged(scroller); }));
	}

	void DemoScrollingPanel::Handle_ButtonBackClick(const Button& button)
	{
		RequestBackNavigation.Trigger(this);
	}

	void DemoScrollingPanel::Handle_SliderButtonCountValueChanged(const Slider& slider)
	{
		std::cout << "Slider '" + slider.GetName() + "' Value Changed. New Value: " << slider.GetValue() << std::endl;
	}

	void DemoScrollingPanel::Handle_ScrollerScrollRatioChanged(const Scroller& scroller)
	{
		std::cout << "Scroller '" + scroller.GetName() + "' Scroll Ratio Changed. New Value: "
				  << scroller.GetScrollRatio() << std::endl;
	}
} // namespace onion::voxel
