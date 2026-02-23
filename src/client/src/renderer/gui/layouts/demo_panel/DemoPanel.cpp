#include "DemoPanel.hpp"

namespace onion::voxel
{

	DemoPanel::DemoPanel(const std::string& name)
		: GuiElement(name), m_Button("DemoButton"), m_Sprite("DemoSprite", m_SpritePath)
	{
		m_Button.SetPosition(0, 0);
		m_Button.SetSize({400.f, 40.f});
		m_Button.SetText("Singleplayer");
		m_Button.SetEnabled(true);
		m_Button.SetScaleUpOnHover(false);
	}

	void DemoPanel::Render()
	{
		// ---- Render Button ----
		float buttonScaleFactorY = 0.08f;
		float buttonXRatio = 0.5f;
		float buttonYRatio = 0.5f;

		const glm::vec2 buttonPos{s_ScreenWidth * buttonXRatio, s_ScreenHeight * buttonYRatio};
		float buttonHeight = s_ScreenHeight * buttonScaleFactorY;
		const glm::vec2 buttonSize{buttonHeight * 10, buttonHeight};

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
	}

	void DemoPanel::Initialize()
	{
		m_Button.Initialize();
		m_Sprite.Initialize();
		SetInitState(true);
	}

	void DemoPanel::Delete()
	{
		m_Button.Delete();
		m_Sprite.Delete();
		SetDeletedState(true);
	}

} // namespace onion::voxel
