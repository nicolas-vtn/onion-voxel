#include "SingleplayerPanel.hpp"

#include <renderer/gui/LayoutHelper.hpp>

namespace onion::voxel
{
	SingleplayerPanel::SingleplayerPanel(const std::string& name)
		: GuiElement(name), m_LabelTitle("Title"), m_TextFieldFilter("Filter"), m_Scroller("Scroller"),
		  m_ButtonBack("Back"), m_ButtonCreateNewWorld("Create New World"),
		  m_ButtonPlaySelectedWorld("Play Selected World"), m_ButtonDeleteSelectedWorld("Delete Selected World"),
		  m_ButtonEdit("Edit"), m_ButtonReCreateSelectedWorld("Re-Create Selected World")
	{
		SubscribeToControlEvents();

		m_LabelTitle.SetText("Singleplayer");
		m_LabelTitle.SetTextAlignment(Font::eTextAlignment::Center);

		m_TextFieldFilter.SetPlaceholderText("Filter...");

		m_ButtonPlaySelectedWorld.SetText("Play Selected World");
		m_ButtonPlaySelectedWorld.SetEnabled(false);

		m_ButtonCreateNewWorld.SetText("Create New World");
		m_ButtonCreateNewWorld.SetEnabled(false);

		m_ButtonEdit.SetText("Edit");
		m_ButtonEdit.SetEnabled(false);

		m_ButtonDeleteSelectedWorld.SetText("Delete");
		m_ButtonDeleteSelectedWorld.SetEnabled(false);

		m_ButtonReCreateSelectedWorld.SetText("Re-Create");
		m_ButtonReCreateSelectedWorld.SetEnabled(false);

		m_ButtonBack.SetText("Back");
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
		glm::vec2 controlsSizeRatio{800.f / 1920.f, 0.08f};
		glm::vec2 controlsSize{controlsSizeRatio.x * s_ScreenWidth, controlsSizeRatio.y * s_ScreenHeight};
		int centerX = s_ScreenWidth / 2;

		// ---- Render Title ----
		float titleYOffsetRatio = (73.f - 23.f) / 1009.f;
		float textHeight = s_ScreenHeight * (28.f / 1009.f);
		m_LabelTitle.SetPosition({centerX, s_ScreenHeight * titleYOffsetRatio});
		m_LabelTitle.SetTextHeight(textHeight);
		m_LabelTitle.Render();

		// ---- Render Filter Text Field ----
		float filterYOffsetRatio = (145.f - 23.f) / 1009.f;
		m_TextFieldFilter.SetPosition({centerX, s_ScreenHeight * filterYOffsetRatio});
		m_TextFieldFilter.SetSize(controlsSize);
		m_TextFieldFilter.Render();

		// ---- Render Scroller ----
		float scrollerWidthRatio = 1.f;
		float scrollerHeightRatio = 580.f / 1009.f;
		glm::ivec2 scrollerSize{static_cast<int>(s_ScreenWidth * scrollerWidthRatio),
								static_cast<int>(s_ScreenHeight * scrollerHeightRatio)};
		float scrollCenterYratio = (503.f - 23.f) / 1009.f;
		glm::ivec2 scrollCenter{centerX, static_cast<int>(s_ScreenHeight * scrollCenterYratio)};

		glm::ivec2 scrollerTopLeftCorner{scrollCenter.x - scrollerSize.x / 2, scrollCenter.y - scrollerSize.y / 2};
		glm::ivec2 scrollerBottomRightCorner{scrollCenter.x + scrollerSize.x / 2, scrollCenter.y + scrollerSize.y / 2};

		// Create a Layout for the World Tiles
		int rows = 1;
		int verticalSpacing = static_cast<int>(0.1f * s_ScreenHeight);
		int worldTileHeight = static_cast<int>(0.1f * s_ScreenHeight);
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

		// Create Layouts for buttons
		glm::ivec2 tableSize{static_cast<int>(1232.f / 1920.f * s_ScreenWidth),
							 static_cast<int>(82.f / 1009.f * s_ScreenHeight)};
		int horizontalSpacing = static_cast<int>(std::round(30.f / 1920.f * s_ScreenWidth));

		TableLayout layoutButtonsTop = LayoutHelper::CreateTableLayout(1, 2, tableSize, horizontalSpacing, 0);
		glm::ivec2 layoutButtonsTop_TopLeftCorner{centerX - (tableSize.x / 2),
												  static_cast<int>((825.f - 23.f) / 1009.f * s_ScreenHeight)};

		TableLayout layoutButtonsBottom = LayoutHelper::CreateTableLayout(1, 4, tableSize, horizontalSpacing, 0);
		glm::ivec2 layoutButtonsBottom_TopLeftCorner{centerX - (tableSize.x / 2),
													 static_cast<int>((921.f - 23.f) / 1009.f * s_ScreenHeight)};
		// ---- Render Play Selected World Button ----
		glm::ivec2 buttonPos = layoutButtonsTop_TopLeftCorner + layoutButtonsTop.GetElementPosition(0, 0);
		glm::ivec2 buttonSize = layoutButtonsTop.GetCellSize();
		m_ButtonPlaySelectedWorld.SetPosition(buttonPos);
		m_ButtonPlaySelectedWorld.SetSize(buttonSize);
		m_ButtonPlaySelectedWorld.Render();

		// ---- Render Create New World Button ----
		buttonPos = layoutButtonsTop_TopLeftCorner + layoutButtonsTop.GetElementPosition(0, 1);
		buttonSize = layoutButtonsTop.GetCellSize();
		m_ButtonCreateNewWorld.SetPosition(buttonPos);
		m_ButtonCreateNewWorld.SetSize(buttonSize);
		m_ButtonCreateNewWorld.Render();

		// ---- Render Edit Selected World Button ----
		buttonPos = layoutButtonsBottom_TopLeftCorner + layoutButtonsBottom.GetElementPosition(0, 0);
		buttonSize = layoutButtonsBottom.GetCellSize();
		m_ButtonEdit.SetPosition(buttonPos);
		m_ButtonEdit.SetSize(buttonSize);
		m_ButtonEdit.Render();

		// ---- Render Delete Selected World Button ----
		buttonPos = layoutButtonsBottom_TopLeftCorner + layoutButtonsBottom.GetElementPosition(0, 1);
		buttonSize = layoutButtonsBottom.GetCellSize();
		m_ButtonDeleteSelectedWorld.SetPosition(buttonPos);
		m_ButtonDeleteSelectedWorld.SetSize(buttonSize);
		m_ButtonDeleteSelectedWorld.Render();

		// ---- Render Re-Create Selected World Button ----
		buttonPos = layoutButtonsBottom_TopLeftCorner + layoutButtonsBottom.GetElementPosition(0, 2);
		buttonSize = layoutButtonsBottom.GetCellSize();
		m_ButtonReCreateSelectedWorld.SetPosition(buttonPos);
		m_ButtonReCreateSelectedWorld.SetSize(buttonSize);
		m_ButtonReCreateSelectedWorld.Render();

		// ---- Render Back Button ----
		buttonPos = layoutButtonsBottom_TopLeftCorner + layoutButtonsBottom.GetElementPosition(0, 3);
		buttonSize = layoutButtonsBottom.GetCellSize();
		m_ButtonBack.SetPosition(buttonPos);
		m_ButtonBack.SetSize(buttonSize);
		m_ButtonBack.Render();
	}

	void SingleplayerPanel::Initialize()
	{
		m_LabelTitle.Initialize();
		m_TextFieldFilter.Initialize();
		m_Scroller.Initialize();
		m_ButtonBack.Initialize();
		m_ButtonCreateNewWorld.Initialize();
		m_ButtonPlaySelectedWorld.Initialize();
		m_ButtonDeleteSelectedWorld.Initialize();
		m_ButtonEdit.Initialize();
		m_ButtonReCreateSelectedWorld.Initialize();

		SetInitState(true);
	}

	void SingleplayerPanel::Delete()
	{
		m_LabelTitle.Delete();
		m_TextFieldFilter.Delete();
		m_Scroller.Delete();
		m_ButtonBack.Delete();
		m_ButtonCreateNewWorld.Delete();
		m_ButtonPlaySelectedWorld.Delete();
		m_ButtonDeleteSelectedWorld.Delete();
		m_ButtonEdit.Delete();
		m_ButtonReCreateSelectedWorld.Delete();

		SetDeletedState(true);
	}

	void SingleplayerPanel::ReloadTextures()
	{
		m_LabelTitle.ReloadTextures();
		m_TextFieldFilter.ReloadTextures();
		m_Scroller.ReloadTextures();
		m_ButtonBack.ReloadTextures();
		m_ButtonCreateNewWorld.ReloadTextures();
		m_ButtonPlaySelectedWorld.ReloadTextures();
		m_ButtonDeleteSelectedWorld.ReloadTextures();
		m_ButtonEdit.ReloadTextures();
		m_ButtonReCreateSelectedWorld.ReloadTextures();
	}

	void SingleplayerPanel::SubscribeToControlEvents()
	{
		m_EventHandles.push_back(
			m_ButtonBack.OnClick.Subscribe([this](const Button& button) { Handle_ButtonBackClick(button); }));

		m_EventHandles.push_back(m_ButtonCreateNewWorld.OnClick.Subscribe(
			[this](const Button& button) { Handle_ButtonCreateNewWorldClick(button); }));

		m_EventHandles.push_back(m_ButtonPlaySelectedWorld.OnClick.Subscribe(
			[this](const Button& button) { Handle_PlaySelectedWorldClick(button); }));

		m_EventHandles.push_back(m_ButtonDeleteSelectedWorld.OnClick.Subscribe(
			[this](const Button& button) { Handle_ButtonDeleteSelectedWorldClick(button); }));

		m_EventHandles.push_back(
			m_ButtonEdit.OnClick.Subscribe([this](const Button& button) { Handle_ButtonEditClick(button); }));

		m_EventHandles.push_back(m_ButtonReCreateSelectedWorld.OnClick.Subscribe(
			[this](const Button& button) { Handle_ButtonReCreateSelectedWorldClick(button); }));
	}

	void SingleplayerPanel::Handle_ButtonBackClick(const Button& button)
	{
		(void) button;
		EvtRequestBackNavigation.Trigger(this);
	}

	void SingleplayerPanel::Handle_ButtonCreateNewWorldClick(const Button& button)
	{
		(void) button;
		assert(false && "Not implemented yet");
	}

	void SingleplayerPanel::Handle_PlaySelectedWorldClick(const Button& button)
	{
		(void) button;
		assert(false && "Not implemented yet");
	}

	void SingleplayerPanel::Handle_ButtonDeleteSelectedWorldClick(const Button& button)
	{
		(void) button;
		assert(false && "Not implemented yet");
	}

	void SingleplayerPanel::Handle_ButtonEditClick(const Button& button)
	{
		(void) button;
		assert(false && "Not implemented yet");
	}

	void SingleplayerPanel::Handle_ButtonReCreateSelectedWorldClick(const Button& button)
	{
		(void) button;
		assert(false && "Not implemented yet");
	}

} // namespace onion::voxel
