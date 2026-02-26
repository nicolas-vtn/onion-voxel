#include "GuiElement.hpp"

#include <cassert>
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

namespace onion::voxel
{

	// -------- Static Member Definitions --------

	Shader GuiElement::s_ShaderSprites(GetAssetsPath() / "shaders/sprite.vert",
									   GetAssetsPath() / "shaders/sprite.frag");

	Shader GuiElement::s_ShaderNineSliceSprites(GetAssetsPath() / "shaders/nine_slice_sprite.vert",
												GetAssetsPath() / "shaders/nine_slice_sprite.frag");

	Font GuiElement::s_TextFont{(GetMinecraftAssetsPath() / "textures/font/ascii.png").string(), 16, 16};

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

	void GuiElement::SetScreenSize(int screenWidth, int screenHeight)
	{
		s_ScreenWidth = screenWidth;
		s_ScreenHeight = screenHeight;

		s_ProjectionMatrix =
			glm::ortho(0.0f, static_cast<float>(screenWidth), static_cast<float>(screenHeight), 0.0f, -1.0f, 1.0f);

		Font::SetProjectionMatrix(s_ProjectionMatrix);

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
		s_TextFont.Load();
	}

	void GuiElement::Unload()
	{
		// Delete Textures
		s_TextFont.Unload();

		// Delete Shaders
		s_ShaderSprites.Delete();
		s_ShaderNineSliceSprites.Delete();

		Font::StaticShutdown();
	}

	// -------- Protected --------

	void GuiElement::SetInitState(bool state)
	{
		m_HasBeenInit.store(state);
	}

	void GuiElement::SetDeletedState(bool state)
	{
		m_HasBeenDeleted.store(state);
	}

} // namespace onion::voxel
