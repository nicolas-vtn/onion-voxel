#include "DemoPanel.hpp"

namespace onion::voxel
{

	DemoPanel::DemoPanel(const std::string& name)
		: GuiElement(name), m_Button("DemoButton"), m_Sprite("DemoSprite", m_SpritePath, Sprite::eOrigin::Asset),
		  m_Button2("DemoButton2"), m_ButtonMainMenu("MainMenuButton"), m_Checkbox("DemoCheckbox"),
		  m_TextField("DemoTextField"), m_Slider("DemoSlider"), m_ButtonScrollingPanel("DemoScrollingPanelButton"),
		  m_ButtonTextsPanel("DemoTextsPanelButton")
	{

		SubscribeToControlEvents();

		m_Button.SetPosition({400, 400});
		m_Button.SetSize({200.f, 70.f});
		m_Button.SetText("Hover Me !");
		m_Button.SetEnabled(true);

		m_Button2.SetPosition({400, 500});
		m_Button2.SetSize({200.f, 40.f});
		m_Button2.SetText("Dummy");
		m_Button2.SetEnabled(false);

		m_ButtonMainMenu.SetPosition({400, 500});
		m_ButtonMainMenu.SetSize({200.f, 40.f});
		m_ButtonMainMenu.SetText("Main Menu");
		m_ButtonMainMenu.SetEnabled(true);

		m_TextField.SetPosition({400, 600});
		m_TextField.SetSize({200.f, 40.f});
		m_TextField.SetText("");
		m_TextField.SetPlaceholderText("Placeholder text...");

		m_Slider.SetPosition({400, 700});
		m_Slider.SetSize({200.f, 40.f});
		m_Slider.SetValue(0);
		m_Slider.SetMaxValue(100);

		m_ButtonScrollingPanel.SetPosition({400, 500});
		m_ButtonScrollingPanel.SetSize({200.f, 40.f});
		m_ButtonScrollingPanel.SetText("Demo Scrolling");
		m_ButtonScrollingPanel.SetEnabled(true);

		m_ButtonTextsPanel.SetText("Demo Texts");
	}

	DemoPanel::~DemoPanel()
	{
		m_EventHandles.clear();
	}

	void DemoPanel::Render()
	{
		glm::vec2 buttonSizeRatio{0.415f, 0.08f};
		glm::vec2 buttonSize{buttonSizeRatio.x * s_ScreenWidth, buttonSizeRatio.y * s_ScreenHeight};

		// ---- Render Sprite ----
		float spriteXScaleFacor = 0.5f;
		float spriteXRatio = 0.5f;
		float spriteYRatio = 0.15f;
		float aspectRatio = (float) m_Sprite.GetTextureHeight() / m_Sprite.GetTextureWidth();

		const glm::vec2 spritePos{s_ScreenWidth * spriteXRatio, s_ScreenHeight * spriteYRatio};
		float spriteSizeX = s_ScreenWidth * spriteXScaleFacor;
		float spriteSizeY = spriteSizeX * aspectRatio;
		const glm::vec2 spriteSize{spriteSizeX, spriteSizeY};

		m_Sprite.SetPosition(spritePos);
		m_Sprite.SetSize(spriteSize);
		m_Sprite.Render();

		// ---- Render Button ----
		float buttonXPosRatio = 0.5f;
		float buttonYPosRatio = 0.4f;

		const glm::vec2 buttonPos{s_ScreenWidth * buttonXPosRatio, s_ScreenHeight * buttonYPosRatio};

		m_Button.SetPosition(buttonPos);
		m_Button.SetSize(buttonSize);
		m_Button.Render();

		// ---- Render Button 2 ----
		float button2XPosRatio = 0.5f;
		float button2YPosRatio = 0.5f;

		const glm::vec2 button2Pos{s_ScreenWidth * button2XPosRatio, s_ScreenHeight * button2YPosRatio};

		m_Button2.SetPosition(button2Pos);
		m_Button2.SetSize(buttonSize);
		m_Button2.Render();

		// ---- Render Checkbox ----
		float cbPositionYRatio = 0.5f;
		float rightEdgeXButton2 = button2Pos.x + buttonSize.x / 2;
		int margin = 5;
		const glm::vec2 cbSize{buttonSize.y, buttonSize.y};
		const glm::vec2 cbPosition{rightEdgeXButton2 + margin + cbSize.x / 2, s_ScreenHeight * cbPositionYRatio};

		m_Checkbox.SetPosition(cbPosition);
		m_Checkbox.SetSize(cbSize);
		m_Checkbox.Render();

		// ---- Render TextField ----
		float textFieldXRatio = 0.5f;
		float textFieldYRatio = 0.6f;
		const glm::vec2 textFieldPos{s_ScreenWidth * textFieldXRatio, s_ScreenHeight * textFieldYRatio};

		m_TextField.SetPosition(textFieldPos);
		m_TextField.SetSize(buttonSize);
		m_TextField.Render();

		// ---- Render Slider ----
		float sliderXRatio = 0.5f;
		float sliderYRatio = 0.7f;
		const glm::vec2 sliderPos{s_ScreenWidth * sliderXRatio, s_ScreenHeight * sliderYRatio};
		std::string sliderText = "Slider Value: " + std::to_string(m_Slider.GetValue());

		m_Slider.SetPosition(sliderPos);
		m_Slider.SetSize(buttonSize);
		m_Slider.SetText(sliderText);
		m_Slider.Render();

		// ---- Render Button Demo Scrolling Panel ----
		glm::ivec2 smallButtonSize{buttonSize.x / 2.f, s_ControlHeight};
		float buttonScrollingPanelXRatio = 0.35f;
		float buttonScrollingPanelYRatio = 0.8f;
		const glm::vec2 buttonScrollingPanelPos{s_ScreenWidth * buttonScrollingPanelXRatio,
												s_ScreenHeight * buttonScrollingPanelYRatio};
		m_ButtonScrollingPanel.SetPosition(buttonScrollingPanelPos);
		m_ButtonScrollingPanel.SetSize(smallButtonSize);
		m_ButtonScrollingPanel.Render();

		// ---- Render Button Demo Texts Panel ----
		float buttonTextsPanelXRatio = 0.65f;
		float buttonTextsPanelYRatio = 0.8f;
		const glm::vec2 buttonTextsPanelPos{s_ScreenWidth * buttonTextsPanelXRatio,
											s_ScreenHeight * buttonTextsPanelYRatio};
		m_ButtonTextsPanel.SetPosition(buttonTextsPanelPos);
		m_ButtonTextsPanel.SetSize(smallButtonSize);
		m_ButtonTextsPanel.Render();

		// ---- Render Pink Rotating Background ----
		float rotationSpeed = 20.f; // degrees per second
		float rotationAngle = static_cast<float>(fmod(glfwGetTime() * rotationSpeed, 360.f));
		glm::ivec2 bgPosition = {s_ScreenWidth / 1.2f, s_ScreenHeight / 1.2f};
		glm::ivec2 bgSize = {s_ScreenWidth / 10.f, s_ScreenHeight / 10.f};
		glm::vec4 bgColor = glm::vec4(1.f, 0.4f, 0.8f, 0.5f);
		ColoredBackground::Options bgOptions;
		bgOptions.Position = bgPosition;
		bgOptions.Size = bgSize;
		bgOptions.Color = bgColor;
		bgOptions.RotationDegrees = rotationAngle;
		bgOptions.ZOffset = 0.1f;
		ColoredBackground::Render(bgOptions);

		// ---- Render Main Menu Button ----
		float buttonMainMenuXPosRatio = 0.5f;
		float buttonMainMenuYPosRatio = 0.9f;

		const glm::vec2 buttonMainMenuPos{s_ScreenWidth * buttonMainMenuXPosRatio,
										  s_ScreenHeight * buttonMainMenuYPosRatio};

		m_ButtonMainMenu.SetPosition(buttonMainMenuPos);
		m_ButtonMainMenu.SetSize(buttonSize);
		m_ButtonMainMenu.Render();
	}

	void DemoPanel::Initialize()
	{
		m_Button.Initialize();
		m_Button2.Initialize();
		m_ButtonScrollingPanel.Initialize();
		m_ButtonTextsPanel.Initialize();
		m_ButtonMainMenu.Initialize();
		m_Sprite.Initialize();
		m_Checkbox.Initialize();
		m_TextField.Initialize();
		m_Slider.Initialize();

		SetInitState(true);
	}

	void DemoPanel::Delete()
	{
		m_Button.Delete();
		m_Button2.Delete();
		m_ButtonMainMenu.Delete();
		m_ButtonScrollingPanel.Delete();
		m_ButtonTextsPanel.Delete();
		m_Sprite.Delete();
		m_Checkbox.Delete();
		m_TextField.Delete();
		m_Slider.Delete();

		SetDeletedState(true);
	}

	void DemoPanel::ReloadTextures()
	{
		m_Button.ReloadTextures();
		m_Button2.ReloadTextures();
		m_ButtonMainMenu.ReloadTextures();
		m_ButtonScrollingPanel.ReloadTextures();
		m_ButtonTextsPanel.ReloadTextures();
		m_Sprite.ReloadTextures();
		m_Checkbox.ReloadTextures();
		m_TextField.ReloadTextures();
		m_Slider.ReloadTextures();
	}

	void DemoPanel::SubscribeToControlEvents()
	{
		m_EventHandles.push_back(
			m_Button.OnClick.Subscribe([this](const Button& button) { Handle_ButtonClick(button); }));

		m_EventHandles.push_back(
			m_Button.OnHoverEnter.Subscribe([this](const Button& button) { Handle_ButtonHoverEnter(button); }));

		m_EventHandles.push_back(
			m_Button.OnHoverLeave.Subscribe([this](const Button& button) { Handle_ButtonHoverLeave(button); }));

		m_EventHandles.push_back(
			m_ButtonMainMenu.OnClick.Subscribe([this](const Button& button) { Handle_ButtonMainMenuClick(button); }));

		m_EventHandles.push_back(m_Checkbox.OnCheckedChanged.Subscribe([this](const Checkbox& checkbox)
																	   { Handle_CheckboxCheckedChanged(checkbox); }));
		m_EventHandles.push_back(
			m_Slider.OnValueChanged.Subscribe([this](const Slider& slider) { Handle_SliderValueChanged(slider); }));

		m_EventHandles.push_back(m_ButtonScrollingPanel.OnClick.Subscribe(
			[this](const Button& button) { Handle_ButtonScrollingPanelClick(button); }));

		m_EventHandles.push_back(m_ButtonTextsPanel.OnClick.Subscribe([this](const Button& button)
																	  { Handle_ButtonTextsPanelClick(button); }));
	}

	void DemoPanel::Handle_CheckboxCheckedChanged(const Checkbox& checkbox)
	{
		std::cout << "Checkbox '" + checkbox.GetName() + "' Checked Changed. New Value: " << checkbox.IsChecked()
				  << std::endl;

		m_Button2.SetEnabled(checkbox.IsChecked());
	}

	void DemoPanel::Handle_SliderValueChanged(const Slider& slider)
	{
		std::cout << "Slider '" + slider.GetName() + "' Value Changed. New Value: " << slider.GetValue() << std::endl;
	}

	void DemoPanel::Handle_ButtonScrollingPanelClick(const Button& button)
	{
		(void) button; // Unused parameter
		RequestMenuNavigation.Trigger({this, eMenu::DemoScrollingPanel});
	}

	void DemoPanel::Handle_ButtonTextsPanelClick(const Button& button)
	{
		(void) button; // Unused parameter
		RequestMenuNavigation.Trigger({this, eMenu::DemoTextsPanel});
	}

	void DemoPanel::Handle_ButtonClick(const Button& button)
	{
		std::cerr << "Button '" + button.GetName() + "' Clicked." << std::endl;
	}

	void DemoPanel::Handle_ButtonHoverEnter(const Button& button)
	{
		std::cout << "Button '" + button.GetName() + "' Hover Enter." << std::endl;
	}

	void DemoPanel::Handle_ButtonHoverLeave(const Button& button)
	{
		std::cout << "Button '" + button.GetName() + "' Hover Leave." << std::endl;
	}

	void DemoPanel::Handle_ButtonMainMenuClick(const Button& button)
	{
		std::cerr << "Button '" + button.GetName() + "' Clicked." << std::endl;
		RequestMenuNavigation.Trigger({this, eMenu::MainMenu});
	}

} // namespace onion::voxel
