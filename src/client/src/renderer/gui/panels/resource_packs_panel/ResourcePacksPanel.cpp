#include "ResourcePacksPanel.hpp"

#include "../../../Variables.hpp"
#include "../../LayoutHelper.hpp"

#include <windows.h>

namespace onion::voxel
{
	ResourcePacksPanel::ResourcePacksPanel(const std::string& name)
		: GuiElement(name), m_Title_Label("ResourcePacksTitle_Label"), m_OpenPackFolder_Button("OpenPackFolder_Button"),
		  m_Done_Button("Done_Button")
	{
		SubscribeToControlEvents();

		m_Title_Label.SetText("Select Resource Packs");
		m_Title_Label.SetTextAlignment(Font::eTextAlignment::Center);

		m_OpenPackFolder_Button.SetText("Open Pack Folder");

		m_Done_Button.SetText("Done");
	}

	ResourcePacksPanel::~ResourcePacksPanel()
	{
		m_EventHandles.clear();
	}

	void ResourcePacksPanel::Render()
	{
		if (s_IsBackPressed)
		{
			RequestBackNavigation.Trigger(this);
			return;
		}

		// ---- Constants for Layout ----
		glm::vec2 buttonSizeRatio{0.415f, 0.08f};
		glm::vec2 buttonSize{buttonSizeRatio.x * s_ScreenWidth, buttonSizeRatio.y * s_ScreenHeight};
		float buttonYSpacingRatio = 94.f / 1009.f;
		float firstButtonYPosRatio = 486.f / 1009.f;
		float middleX = s_ScreenWidth * 0.5f;

		// ---- Render Menu Title ----
		constexpr float menuYOffsetRatio = 0.0320f;
		glm::vec2 textPosition = {s_ScreenWidth / 2, s_ScreenHeight * menuYOffsetRatio};
		float textHeight = s_ScreenHeight * (30.f / 1009.f);

		m_Title_Label.SetPosition(textPosition);
		m_Title_Label.SetTextHeight(textHeight);
		m_Title_Label.Render();

		// ---- Prepare Layout for first 2 buttons ----
		constexpr float tablesWidthRatio = 1229.f / 1920.f;
		float tablesWidth = s_ScreenWidth * tablesWidthRatio;
		constexpr float table1HeightRatio = 79.f / 1009.f;
		float table1Height = s_ScreenHeight * table1HeightRatio;

		constexpr float horizontalSpacingRatios = 33.f / 1920.f;
		float horizontalSpacings = s_ScreenWidth * horizontalSpacingRatios;
		constexpr float verticalSpacingRatios = 17.f / 1009.f;
		float verticalSpacings = s_ScreenHeight * verticalSpacingRatios;

		constexpr float table1TopRatio = (930.f - 23.f) / 1009.f;

		glm::ivec2 topLeftOfTable1{s_ScreenWidth * 0.5 - (tablesWidth / 2), s_ScreenHeight * table1TopRatio};

		TableLayout tableLayout1 = LayoutHelper::CreateTableLayout(
			1, 2, glm::ivec2(tablesWidth, table1Height), horizontalSpacings, verticalSpacings);

		const glm::ivec2 cellSize1 = tableLayout1.GetCellSize();

		// ---- Render FOV Button ----
		glm::ivec2 relativeButtonPos1 = tableLayout1.GetElementPosition(0, 0);
		m_OpenPackFolder_Button.SetPosition(topLeftOfTable1 + relativeButtonPos1);
		m_OpenPackFolder_Button.SetSize(cellSize1);
		m_OpenPackFolder_Button.Render();

		// ---- Render Online Button ----
		relativeButtonPos1 = tableLayout1.GetElementPosition(0, 1);
		m_Done_Button.SetPosition(topLeftOfTable1 + relativeButtonPos1);
		m_Done_Button.SetSize(cellSize1);
		m_Done_Button.Render();
	}

	void ResourcePacksPanel::Initialize()
	{
		m_Title_Label.Initialize();
		m_OpenPackFolder_Button.Initialize();
		m_Done_Button.Initialize();

		SetInitState(true);
	}

	void ResourcePacksPanel::Delete()
	{
		m_Title_Label.Delete();
		m_OpenPackFolder_Button.Delete();
		m_Done_Button.Delete();

		SetDeletedState(true);
	}

	void ResourcePacksPanel::SubscribeToControlEvents()
	{
		m_EventHandles.push_back(m_OpenPackFolder_Button.OnClick.Subscribe([this](const Button& sender)
																		   { Handle_OpenPackFolder_Click(sender); }));

		m_EventHandles.push_back(
			m_Done_Button.OnClick.Subscribe([this](const Button& sender) { Handle_Done_Click(sender); }));
	}

	void ResourcePacksPanel::Handle_OpenPackFolder_Click(const Button& sender)
	{
		std::filesystem::path resourcePacksFolderPath = GetResourcePacksDirectory();
		ShellExecuteA(
			nullptr, "open", "explorer.exe", resourcePacksFolderPath.string().c_str(), nullptr, SW_SHOWNORMAL);
	}

	void ResourcePacksPanel::Handle_Done_Click(const Button& sender)
	{
		RequestBackNavigation.Trigger(this);
	}

} // namespace onion::voxel
