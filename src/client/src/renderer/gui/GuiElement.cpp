#include "GuiElement.hpp"

#include <cassert>
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

namespace onion::voxel
{

	std::string GetMenuName(eMenu menu)
	{
		switch (menu)
		{
			case eMenu::DemoPanel:
				return "Demo Panel";
			case eMenu::MainMenu:
				return "Main Menu";
			case eMenu::Singleplayer:
				return "Singleplayer";
			case eMenu::Multiplayer:
				return "Multiplayer";
			case eMenu::Options:
				return "Settings";
			case eMenu::Gameplay:
				return "Gameplay";
			case eMenu::Pause:
				return "Pause Menu";
			case eMenu::MusicAndSounds:
				return "Music & Sounds";
			case eMenu::ResourcePacks:
				return "Resource Packs";
			case eMenu::Controls:
				return "Controls";
			default:
				return "None";
		}
	}

	// -------- Static Member Definitions --------

	Shader GuiElement::s_ShaderSprites(AssetsManager::GetShadersDirectory() / "sprite.vert",
									   AssetsManager::GetShadersDirectory() / "sprite.frag");

	Shader GuiElement::s_ShaderNineSliceSprites(AssetsManager::GetShadersDirectory() / "nine_slice_sprite.vert",
												AssetsManager::GetShadersDirectory() / "nine_slice_sprite.frag");
	Font GuiElement::s_TextFont;

	Event<const CursorStyle&> GuiElement::EvtRequestCursorStyleChange;

	glm::mat4 GuiElement::s_ProjectionMatrix{1.0f};
	int GuiElement::s_ScreenWidth = 800;
	int GuiElement::s_ScreenHeight = 600;

	float GuiElement::s_CanvasOffsetX = 0.f;
	float GuiElement::s_CanvasOffsetY = 0.f;

	float GuiElement::s_TextHeight = 16.0f;
	int GuiElement::s_ControlHeight = 20;
	int GuiElement::s_CenterX = 400;

	std::shared_ptr<InputsSnapshot> GuiElement::s_InputsSnapshot = nullptr;

	// -------- Constructor / Destructor --------

	GuiElement::GuiElement(const std::string& name) : m_Name(name) {}

	GuiElement::~GuiElement()
	{
		if (m_HasBeenInit.load() && !m_HasBeenDeleted.load())
		{
			std::string errorMessage =
				"The GuiElement '" + m_Name + "' destructor was called before GuiElement::Delete()";

			std::cerr << errorMessage << std::endl;
			assert(false);
		}
	}

	// -------- Public API --------

	void GuiElement::ReloadStaticTextures()
	{
		s_TextFont.Reload();
	}

	std::string GuiElement::GetName() const
	{
		return m_Name;
	}

	void GuiElement::SetName(const std::string& name)
	{
		m_Name = name;
	}

	GuiElement::Visibility GuiElement::GetVisibility() const
	{
		return m_Visibility;
	}

	void GuiElement::SetVisibility(const Visibility& visibility)
	{
		m_Visibility = visibility;
	}

	void GuiElement::SetScreenSize(int screenWidth, int screenHeight)
	{
		s_ScreenWidth = screenWidth;
		s_ScreenHeight = screenHeight;

		s_ProjectionMatrix =
			glm::ortho(0.0f, static_cast<float>(screenWidth), static_cast<float>(screenHeight), 0.0f, -1.0f, 1.0f);

		// Resolve active scale: 0 = auto (largest integer that fits the canvas).
		int userScale = s_GuiScale.load();
		int activeScale = (userScale > 0)
			? userScale
			: std::max(1, std::min(screenWidth / k_LogicalWidth, screenHeight / k_LogicalHeight));
		s_ActiveGuiScale = activeScale;

		// Center the logical canvas inside the physical framebuffer.
		s_CanvasOffsetX = (screenWidth  - k_LogicalWidth  * activeScale) / 2.f;
		s_CanvasOffsetY = (screenHeight - k_LogicalHeight * activeScale) / 2.f;

		// Derived helpers — fixed multiples of the active scale, never fractional.
		s_TextHeight    = 8.f * activeScale;
		s_ControlHeight = 20  * activeScale;
		s_CenterX       = screenWidth / 2;

		Font::SetProjectionMatrix(s_ProjectionMatrix);
		ColoredBackground::SetProjectionMatrix(s_ProjectionMatrix);

		s_ShaderSprites.Use();
		s_ShaderSprites.setMat4("uProjection", s_ProjectionMatrix);

		s_ShaderNineSliceSprites.Use();
		s_ShaderNineSliceSprites.setMat4("uProjection", s_ProjectionMatrix);
	}

	float GuiElement::Lx(float logicalX)
	{
		return s_CanvasOffsetX + logicalX * s_ActiveGuiScale.load();
	}

	float GuiElement::Ly(float logicalY)
	{
		return s_CanvasOffsetY + logicalY * s_ActiveGuiScale.load();
	}

	glm::vec2 GuiElement::L(float logicalX, float logicalY)
	{
		return {Lx(logicalX), Ly(logicalY)};
	}

	float GuiElement::Ls(float logicalSize)
	{
		return logicalSize * s_ActiveGuiScale.load();
	}

	void GuiElement::SetInputsSnapshot(std::shared_ptr<InputsSnapshot> inputsSnapshot)
	{
		s_InputsSnapshot = inputsSnapshot;
	}

	bool GuiElement::AreKeyInputsValid()
	{
		return EngineContext::Get().FrameCount >= s_KeyInputsValidFromFrame;
	}

	bool GuiElement::IsBackPressed()
	{
		return AreKeyInputsValid() && EngineContext::Get().Keys->GetKeyState(eAction::CloseMenu).IsPressed;
	}

	void GuiElement::Load()
	{
		Font::StaticInitialize();
		s_TextFont.Load();
		ReloadStaticTextures();
	}

	void GuiElement::Unload()
	{
		// Delete Textures
		s_TextFont.Unload();

		// Delete Shaders
		s_ShaderSprites.Delete();
		s_ShaderNineSliceSprites.Delete();

		Font::StaticShutdown();
		ColoredBackground::StaticShutdown();
	}

	// -------- Protected --------

	void GuiElement::SetInitState(bool state)
	{
		m_HasBeenInit.store(state);
	}

	bool GuiElement::HasBeenInit() const
	{
		return m_HasBeenInit;
	}

	void GuiElement::SetDeletedState(bool state)
	{
		m_HasBeenDeleted.store(state);
	}

	bool GuiElement::HasBeenDeleted() const
	{
		return m_HasBeenDeleted;
	}

	void GuiElement::SetGuiScale(int scale)
	{
		s_GuiScale = scale;
	}

	int GuiElement::GetGuiScale()
	{
		return s_GuiScale;
	}

} // namespace onion::voxel
