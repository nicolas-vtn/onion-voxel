#include "SingleplayerPanel.hpp"

#include <renderer/gui/LayoutHelper.hpp>

namespace onion::voxel
{
	SingleplayerPanel::SingleplayerPanel(const std::string& name)
		: GuiElement(name), m_LabelTitle("Title"), m_TextFieldFilter("Filter"), m_Scroller("Scroller"),
		  m_ButtonBack("Back"), m_ButtonCreateNewWorld("Create New World"), m_PlaySelectedWorld("Play Selected World"),
		  m_DeleteSelectedWorld("Delete Selected World")
	{
		SubscribeToControlEvents();

		m_LabelTitle.SetText("Singleplayer");
		m_LabelTitle.SetTextAlignment(Font::eTextAlignment::Center);

		m_TextFieldFilter.SetPlaceholderText("Filter...");

		m_ButtonBack.SetText("Back");

		m_ButtonCreateNewWorld.SetText("Create New World");
		m_ButtonCreateNewWorld.SetEnabled(false);

		m_PlaySelectedWorld.SetText("Play Selected World");
		m_PlaySelectedWorld.SetEnabled(false);

		m_DeleteSelectedWorld.SetText("Delete Selected World");
		m_DeleteSelectedWorld.SetEnabled(false);
	}

	SingleplayerPanel::~SingleplayerPanel()
	{
		m_EventHandles.clear();
	}

	void SingleplayerPanel::Render()
	{
		if (s_IsBackPressed)
		{
			EvtRequestBackNavigation.Trigger(this);
			return;
		}

		// Constants for Layout
		glm::vec2 controlsSizeRatio{0.415f, 0.08f};
		glm::vec2 controlsSize{controlsSizeRatio.x * s_ScreenWidth, controlsSizeRatio.y * s_ScreenHeight};
		int centerX = s_ScreenWidth / 2;

		// ---- Render Title ----
		float titleYOffsetRatio = (100.f - 23.f) / 1009.f;
		float textHeight = s_ScreenHeight * (21.f / 1009.f);
		m_LabelTitle.SetPosition({centerX, s_ScreenHeight * titleYOffsetRatio});
		m_LabelTitle.SetTextHeight(textHeight);
		m_LabelTitle.Render();

		// ---- Render Filter Text Field ----
		float filterYOffsetRatio = (150.f - 23.f) / 1009.f;
		m_TextFieldFilter.SetPosition({centerX, s_ScreenHeight * filterYOffsetRatio});
		m_TextFieldFilter.SetSize(controlsSize);
		m_TextFieldFilter.Render();

		// ---- Render Scroller ----
		float scrollerWidthRatio = 1.f;
		float scrollerHeightRatio = 0.6f;
		glm::ivec2 scrollerSize{static_cast<int>(s_ScreenWidth * scrollerWidthRatio),
								static_cast<int>(s_ScreenHeight * scrollerHeightRatio)};
		glm::ivec2 scrollCenter{centerX, s_ScreenHeight / 2};

		glm::ivec2 scrollerTopLeftCorner{scrollCenter.x - scrollerSize.x / 2, scrollCenter.y - scrollerSize.y / 2};
		glm::ivec2 scrollerBottomRightCorner{scrollCenter.x + scrollerSize.x / 2, scrollCenter.y + scrollerSize.y / 2};

		// Create a Layout for the World Tiles
		int rows = 1;
		int verticalSpacing = 0.1f * s_ScreenHeight;
		int worldTileHeight = 0.1f * s_ScreenHeight;
		int totalHeight = rows * worldTileHeight + (rows - 1) * verticalSpacing;

		m_Scroller.SetScrollAreaHeight(totalHeight);
		m_Scroller.SetTopLeftCorner(scrollerTopLeftCorner);
		m_Scroller.SetBottomRightCorner(scrollerBottomRightCorner);

		m_Scroller.Render();

		// ---- Start Cissoring for Scroller ----
		m_Scroller.StartCissoring();

		// ---- Render World Tiles ----
		// TODO : ...

		// ---- Stop Cissoring for Scroller ----
		m_Scroller.StopCissoring();

		// ---- Render Create New World Button ----
		// TODO : ...

		// ---- Render Play Selected World Button ----
		// TODO : ...

		// ---- Render Delete Selected World Button ----
		// TODO : ...

		// ---- Render Back Button ----
		// TODO : ...
	}

	void SingleplayerPanel::Initialize()
	{
		m_LabelTitle.Initialize();
		m_TextFieldFilter.Initialize();
		m_Scroller.Initialize();
		m_ButtonBack.Initialize();
		m_ButtonCreateNewWorld.Initialize();
		m_PlaySelectedWorld.Initialize();
		m_DeleteSelectedWorld.Initialize();

		SetInitState(true);
	}

	void SingleplayerPanel::Delete()
	{
		m_LabelTitle.Delete();
		m_TextFieldFilter.Delete();
		m_Scroller.Delete();
		m_ButtonBack.Delete();
		m_ButtonCreateNewWorld.Delete();
		m_PlaySelectedWorld.Delete();
		m_DeleteSelectedWorld.Delete();

		SetDeletedState(true);
	}

	void SingleplayerPanel::ReloadTextures()
	{
		m_LabelTitle.ReloadTextures();
		m_TextFieldFilter.ReloadTextures();
		m_Scroller.ReloadTextures();
		m_ButtonBack.ReloadTextures();
		m_ButtonCreateNewWorld.ReloadTextures();
		m_PlaySelectedWorld.ReloadTextures();
		m_DeleteSelectedWorld.ReloadTextures();
	}

	void SingleplayerPanel::SubscribeToControlEvents()
	{
		m_EventHandles.push_back(
			m_ButtonBack.OnClick.Subscribe([this](const Button& button) { Handle_ButtonBackClick(button); }));

		m_EventHandles.push_back(m_ButtonCreateNewWorld.OnClick.Subscribe(
			[this](const Button& button) { Handle_ButtonCreateNewWorldClick(button); }));

		m_EventHandles.push_back(m_PlaySelectedWorld.OnClick.Subscribe([this](const Button& button)
																	   { Handle_PlaySelectedWorldClick(button); }));

		m_EventHandles.push_back(m_DeleteSelectedWorld.OnClick.Subscribe(
			[this](const Button& button) { Handle_ButtonDeleteSelectedWorldClick(button); }));
	}

	void SingleplayerPanel::Handle_ButtonBackClick(const Button& button)
	{
		EvtRequestBackNavigation.Trigger(this);
	}

	void SingleplayerPanel::Handle_ButtonCreateNewWorldClick(const Button& button)
	{
		assert(false && "Not implemented yet");
	}

	void SingleplayerPanel::Handle_PlaySelectedWorldClick(const Button& button)
	{
		assert(false && "Not implemented yet");
	}

	void SingleplayerPanel::Handle_ButtonDeleteSelectedWorldClick(const Button& button)
	{
		assert(false && "Not implemented yet");
	}

} // namespace onion::voxel
