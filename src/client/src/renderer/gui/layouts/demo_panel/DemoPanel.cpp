#include "DemoPanel.hpp"

namespace onion::voxel
{

	DemoPanel::DemoPanel(const std::string& name)
		: GuiElement(name), m_Button("DemoButton"), m_Sprite("DemoSprite", m_SpritePath), m_Button2("DemoButton2")
	{
		m_Button.SetPosition({400, 400});
		m_Button.SetSize({700.f, 70.f});
		m_Button.SetText("Text Button !");
		m_Button.SetEnabled(true);

		m_Button2.SetPosition({0, 50});
		m_Button2.SetSize({200.f, 40.f});
		m_Button2.SetText("Multi");
		m_Button2.SetEnabled(true);

		m_HandleButtonClick = m_Button.OnClick.Subscribe([this](const Button& button) { HandleButtonClick(button); });
		m_HandleButtonHoverEnter =
			m_Button.OnHoverEnter.Subscribe([this](const Button& button) { HandleButtonHoverEnter(button); });
		m_HandleButtonHoverLeave =
			m_Button.OnHoverLeave.Subscribe([this](const Button& button) { HandleButtonHoverLeave(button); });
	}

	void DemoPanel::Render()
	{
		// ---- Render Button ----
		float buttonScaleFactorY = 0.08f;
		float buttonXRatio = 0.5f;
		float buttonYRatio = 0.5f;

		const glm::vec2 buttonPos{s_ScreenWidth * buttonXRatio, s_ScreenHeight * buttonYRatio};
		float buttonHeight = s_ScreenHeight * buttonScaleFactorY;
		const glm::vec2 buttonSize{200, 40};

		//m_Button.SetPosition(buttonPos);
		//m_Button.SetSize(buttonSize);
		m_Button.Render();

		//// ---- Render Sprite ----
		//float spriteXScaleFacor = 0.5f;
		//float spriteXRatio = 0.5f;
		//float spriteYRatio = 0.2f;
		//float aspectRatio = (float) m_Sprite.GetTextureHeight() / m_Sprite.GetTextureWidth();

		//const glm::vec2 spritePos{s_ScreenWidth * spriteXRatio, s_ScreenHeight * spriteYRatio};
		//float spriteSizeX = s_ScreenWidth * spriteXScaleFacor;
		//float spriteSizeY = spriteSizeX * aspectRatio;
		//const glm::vec2 spriteSize{spriteSizeX, spriteSizeY};

		//m_Sprite.SetPosition(spritePos);
		//m_Sprite.SetSize(spriteSize);
		//m_Sprite.Render();

		//// ---- Render Button 2 ----
		//float button2ScaleFactorY = 0.08f;
		//float button2XRatio = 0.5f;
		//float button2YRatio = 0.6f;

		//const glm::vec2 button2Pos{s_ScreenWidth * button2XRatio, s_ScreenHeight * button2YRatio};
		//float button2Height = s_ScreenHeight * button2ScaleFactorY;
		//const glm::vec2 button2Size{button2Height * 5, button2Height};

		//m_Button2.SetPosition(button2Pos);
		//m_Button2.SetSize(button2Size);
		//m_Button2.Render();
	}

	void DemoPanel::Initialize()
	{
		m_Button.Initialize();
		m_Button2.Initialize();
		m_Sprite.Initialize();
		SetInitState(true);
	}

	void DemoPanel::Delete()
	{
		m_Button.Delete();
		m_Button2.Delete();
		m_Sprite.Delete();
		SetDeletedState(true);
	}

	void DemoPanel::HandleButtonClick(const Button& button)
	{
		std::cout << "Button '" + button.GetName() + "' Clicked." << std::endl;
	}

	void DemoPanel::HandleButtonHoverEnter(const Button& button)
	{
		std::cout << "Button '" + button.GetName() + "' Hover Enter." << std::endl;
	}

	void DemoPanel::HandleButtonHoverLeave(const Button& button)
	{
		std::cout << "Button '" + button.GetName() + "' Hover Leave." << std::endl;
	}

} // namespace onion::voxel
