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
	Font GuiElement::s_TextFont{s_PathFont, 16, 16};

	Event<const CursorStyle&> GuiElement::RequestCursorStyleChange;

	glm::mat4 GuiElement::s_ProjectionMatrix{1.0f};
	int GuiElement::s_ScreenWidth = 800;
	int GuiElement::s_ScreenHeight = 600;

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

		Font::SetProjectionMatrix(s_ProjectionMatrix);
		ColoredBackground::SetProjectionMatrix(s_ProjectionMatrix);

		s_ShaderSprites.Use();
		s_ShaderSprites.setMat4("uProjection", s_ProjectionMatrix);

		s_ShaderNineSliceSprites.Use();
		s_ShaderNineSliceSprites.setMat4("uProjection", s_ProjectionMatrix);
	}

	void GuiElement::SetInputsSnapshot(std::shared_ptr<InputsSnapshot> inputsSnapshot)
	{
		s_InputsSnapshot = inputsSnapshot;
	}

	void GuiElement::Load()
	{
		Font::StaticInitialize();
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
