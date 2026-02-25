#include "DemoPanel.hpp"

namespace onion::voxel
{

	DemoPanel::DemoPanel(const std::string& name)
		: GuiElement(name), m_Button("DemoButton"), m_Sprite("DemoSprite", m_SpritePath), m_Button2("DemoButton2")
	{
		m_Button.SetPosition({400, 400});
		m_Button.SetSize({200.f, 70.f});
		m_Button.SetText("Singleplayer");
		m_Button.SetEnabled(true);

		m_Button2.SetPosition({400, 500});
		m_Button2.SetSize({200.f, 40.f});
		m_Button2.SetText("Multiplayer");
		m_Button2.SetEnabled(true);

		m_HandleButtonClick = m_Button.OnClick.Subscribe([this](const Button& button) { HandleButtonClick(button); });
		m_HandleButtonHoverEnter =
			m_Button.OnHoverEnter.Subscribe([this](const Button& button) { HandleButtonHoverEnter(button); });
		m_HandleButtonHoverLeave =
			m_Button.OnHoverLeave.Subscribe([this](const Button& button) { HandleButtonHoverLeave(button); });
	}

	void DemoPanel::Render()
	{
		glm::vec2 buttonSizeRatio{0.415f, 0.08f};

		glm::vec2 buttonSize{buttonSizeRatio.x * s_ScreenWidth, buttonSizeRatio.y * s_ScreenHeight};

		// ---- Render Button ----
		float buttonScaleFactorY = 0.08f;
		float buttonXPosRatio = 0.5f;
		float buttonYPosRatio = 0.5f;

		const glm::vec2 buttonPos{s_ScreenWidth * buttonXPosRatio, s_ScreenHeight * buttonYPosRatio};

		m_Button.SetPosition(buttonPos);
		m_Button.SetSize(buttonSize);
		m_Button.Render();

		// ---- Render Sprite ----
		float spriteXScaleFacor = 0.5f;
		float spriteXRatio = 0.5f;
		float spriteYRatio = 0.2f;
		float aspectRatio = (float) m_Sprite.GetTextureHeight() / m_Sprite.GetTextureWidth();

		const glm::vec2 spritePos{s_ScreenWidth * spriteXRatio, s_ScreenHeight * spriteYRatio};
		float spriteSizeX = s_ScreenWidth * spriteXScaleFacor;
		float spriteSizeY = spriteSizeX * aspectRatio;
		const glm::vec2 spriteSize{spriteSizeX, spriteSizeY};

		m_Sprite.SetPosition(spritePos);
		m_Sprite.SetSize(spriteSize);
		m_Sprite.Render();

		// ---- Render Button 2 ----
		float button2ScaleFactorY = 0.5f;
		float button2XPosRatio = 0.5f;
		float button2YPosRatio = 0.6f;

		const glm::vec2 button2Pos{s_ScreenWidth * button2XPosRatio, s_ScreenHeight * button2YPosRatio};

		m_Button2.SetPosition(button2Pos);
		m_Button2.SetSize(buttonSize);
		m_Button2.Render();
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
