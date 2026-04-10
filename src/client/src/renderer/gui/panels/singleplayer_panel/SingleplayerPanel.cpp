#include "SingleplayerPanel.hpp"

#include <shared/utils/Utils.hpp>

#include <renderer/gui/LayoutHelper.hpp>

namespace onion::voxel
{
	SingleplayerPanel::SingleplayerPanel(const std::string& name)
		: GuiElement(name), m_LabelTitle("Title"), m_TextFieldFilter("Search..."), m_Scroller("Scroller"),
		  m_ButtonBack("Back"), m_ButtonCreateNewWorld("Create New World"),
		  m_ButtonPlaySelectedWorld("Play Selected World"), m_ButtonDeleteSelectedWorld("Delete Selected World"),
		  m_ButtonEdit("Edit"), m_ButtonRefreshWorldTiles("Re-Create Selected World"),
		  m_LabelDeleteWarning("Delete Warning"), m_LabelDeleteDetails("Delete Details"),
		  m_ButtonDeleteConfirm("Delete Confirm"), m_ButtonDeleteCancel("Delete Cancel"),
		  m_LabelCreateNewWorldTitle("Create New World Title"), m_LabelCreateNewWorldName("Create New World Name"),
		  m_TextFieldCreateNewWorldName("Create New World Name"),
		  m_ButtonCreateNewWorldSelectType("Create New World Select Type"),
		  m_LabelCreateNewWorldSeed("Create New World Seed"), m_TextFieldCreateNewWorldSeed("Create New World Seed"),
		  m_ButtonCreateNewWorldConfirm("Create New World Confirm"),
		  m_ButtonCreateNewWorldCancel("Create New World Cancel"), m_ScrollerCreateNewWorld("Create New World Scroller")
	{
		SubscribeToControlEvents();

		// ---- Initialize Controls World Tiles ----
		{
			m_LabelTitle.SetText("Select World");
			m_LabelTitle.SetTextAlignment(Font::eTextAlignment::Center);

			m_TextFieldFilter.SetPlaceholderText("Search...");

			m_ButtonPlaySelectedWorld.SetText("Play Selected World");
			m_ButtonPlaySelectedWorld.SetEnabled(false);

			m_ButtonCreateNewWorld.SetText("Create New World");

			m_ButtonEdit.SetText("Edit");
			m_ButtonEdit.SetEnabled(false);

			m_ButtonDeleteSelectedWorld.SetText("Delete");
			m_ButtonDeleteSelectedWorld.SetEnabled(false);

			m_ButtonRefreshWorldTiles.SetText("Refresh");

			m_ButtonBack.SetText("Back");
		}

		// ---- Initialize Controls Delete Confirmation -----
		{
			m_LabelDeleteWarning.SetTextAlignment(Font::eTextAlignment::Center);
			m_LabelDeleteDetails.SetTextAlignment(Font::eTextAlignment::Center);

			m_ButtonDeleteConfirm.SetText("Delete");
			m_ButtonDeleteCancel.SetText("Cancel");
		}

		// ---- Initialize Controls Create New World -----
		{
			m_LabelCreateNewWorldTitle.SetText("Create New World");
			m_LabelCreateNewWorldTitle.SetTextAlignment(Font::eTextAlignment::Center);
			m_LabelCreateNewWorldName.SetText("World Name:");
			m_TextFieldCreateNewWorldName.SetPlaceholderText("Enter world name...");
			m_LabelCreateNewWorldSeed.SetText("Seed:");
			m_LabelCreateNewWorldSeed.SetTextAlignment(Font::eTextAlignment::Left);
			m_TextFieldCreateNewWorldSeed.SetPlaceholderText("Enter seed...");
			m_ButtonCreateNewWorldConfirm.SetText("Create");
			m_ButtonCreateNewWorldCancel.SetText("Cancel");
		}
	}

	SingleplayerPanel::~SingleplayerPanel()
	{
		m_EventHandles.clear();
	}

	void SingleplayerPanel::Render()
	{
		switch (m_CurrentRenderModule)
		{
			case eRenderModule::WorldTiles:
				RenderWorldTiles();
				break;
			case eRenderModule::DeleteConfirmation:
				RenderDeleteConfirmation();
				break;
			case eRenderModule::EditWorld:
				RenderEditWorld();
				break;
			case eRenderModule::CreateNewWorld:
				RenderCreateNewWorld();
				break;
			default:
				break;
		}
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
		m_ButtonRefreshWorldTiles.Initialize();

		m_LabelDeleteWarning.Initialize();
		m_LabelDeleteDetails.Initialize();
		m_ButtonDeleteConfirm.Initialize();
		m_ButtonDeleteCancel.Initialize();

		m_LabelCreateNewWorldTitle.Initialize();
		m_LabelCreateNewWorldName.Initialize();
		m_TextFieldCreateNewWorldName.Initialize();
		m_ButtonCreateNewWorldSelectType.Initialize();
		m_LabelCreateNewWorldSeed.Initialize();
		m_TextFieldCreateNewWorldSeed.Initialize();
		m_ButtonCreateNewWorldConfirm.Initialize();
		m_ButtonCreateNewWorldCancel.Initialize();
		m_ScrollerCreateNewWorld.Initialize();

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
		m_ButtonRefreshWorldTiles.Delete();
		ClearWorldTiles();

		m_LabelDeleteWarning.Delete();
		m_LabelDeleteDetails.Delete();
		m_ButtonDeleteConfirm.Delete();
		m_ButtonDeleteCancel.Delete();

		m_LabelCreateNewWorldTitle.Delete();
		m_LabelCreateNewWorldName.Delete();
		m_TextFieldCreateNewWorldName.Delete();
		m_ButtonCreateNewWorldSelectType.Delete();
		m_LabelCreateNewWorldSeed.Delete();
		m_TextFieldCreateNewWorldSeed.Delete();
		m_ButtonCreateNewWorldConfirm.Delete();
		m_ButtonCreateNewWorldCancel.Delete();
		m_ScrollerCreateNewWorld.Delete();

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
		m_ButtonRefreshWorldTiles.ReloadTextures();
		for (const auto& worldTile : m_WorldTiles)
		{
			worldTile->ReloadTextures();
		}

		m_LabelDeleteWarning.ReloadTextures();
		m_LabelDeleteDetails.ReloadTextures();
		m_ButtonDeleteConfirm.ReloadTextures();
		m_ButtonDeleteCancel.ReloadTextures();

		m_LabelCreateNewWorldTitle.ReloadTextures();
		m_LabelCreateNewWorldName.ReloadTextures();
		m_TextFieldCreateNewWorldName.ReloadTextures();
		m_ButtonCreateNewWorldSelectType.ReloadTextures();
		m_LabelCreateNewWorldSeed.ReloadTextures();
		m_TextFieldCreateNewWorldSeed.ReloadTextures();
		m_ButtonCreateNewWorldConfirm.ReloadTextures();
		m_ButtonCreateNewWorldCancel.ReloadTextures();
		m_ScrollerCreateNewWorld.ReloadTextures();
	}

	void SingleplayerPanel::RefreshWorldTiles()
	{
		ClearWorldTiles();

		std::vector<WorldInfos> worldsInfos = GetWorldsInfos();

		for (const auto& worldInfos : worldsInfos)
		{
			std::filesystem::path thumbnailPath =
				EngineContext::Get().Assets->GetAssetsDirectory() / "textures" / "default_pack.png";
			Texture thumbnailTexture(thumbnailPath);

			std::unique_ptr<WorldTile> worldTile =
				std::make_unique<WorldTile>(worldInfos.Name, worldInfos, std::move(thumbnailTexture));

			worldTile->SetWorldInfos(worldInfos);
			worldTile->Initialize();

			m_EventHandles.push_back(worldTile->EvtTileSelected.Subscribe([this](const WorldTile& tile)
																		  { Handle_WorldTileSelected(tile); }));

			m_EventHandles.push_back(worldTile->EvtTileDoubleClicked.Subscribe(
				[this](const WorldTile& tile) { Handle_WorldTileDoubleClicked(tile); }));

			m_WorldTiles.push_back(std::move(worldTile));
		}

		// Sort World Tiles by last played date (most recent first)
		std::sort(m_WorldTiles.begin(),
				  m_WorldTiles.end(),
				  [](const std::unique_ptr<WorldTile>& a, const std::unique_ptr<WorldTile>& b)
				  { return a->GetWorldInfos().LastPlayedDate > b->GetWorldInfos().LastPlayedDate; });
	}

	void SingleplayerPanel::RenderWorldTiles()
	{
		if (s_IsBackPressed)
		{
			Handle_ButtonBackClick(m_ButtonBack);
			return;
		}

		// Constants for Layout
		glm::vec2 controlsSizeRatio{800.f / 1920.f, 0.08f};
		glm::vec2 controlsSize{controlsSizeRatio.x * s_ScreenWidth, s_ControlHeight};

		// ---- Render Title ----
		float titleYOffsetRatio = (71.f - 23.f) / 1009.f;
		m_LabelTitle.SetPosition({s_CenterX, s_ScreenHeight * titleYOffsetRatio});
		m_LabelTitle.SetTextHeight(s_TextHeight);
		m_LabelTitle.Render();

		// ---- Render Filter Text Field ----
		float filterYOffsetRatio = (147.f - 23.f) / 1009.f;
		m_TextFieldFilter.SetPosition({s_CenterX, s_ScreenHeight * filterYOffsetRatio});
		m_TextFieldFilter.SetSize(controlsSize);
		m_TextFieldFilter.Render();

		// ---- Render Scroller ----
		float scrollerWidthRatio = 1.f;
		float scrollerHeightRatio = 574.f / 1009.f;
		glm::ivec2 scrollerSize{static_cast<int>(s_ScreenWidth * scrollerWidthRatio),
								static_cast<int>(s_ScreenHeight * scrollerHeightRatio)};
		float scrollCenterYratio = (508.f - 23.f) / 1009.f;
		glm::ivec2 scrollCenter{s_CenterX, static_cast<int>(s_ScreenHeight * scrollCenterYratio)};

		glm::ivec2 scrollerTopLeftCorner{scrollCenter.x - scrollerSize.x / 2, scrollCenter.y - scrollerSize.y / 2};
		glm::ivec2 scrollerBottomRightCorner{scrollCenter.x + scrollerSize.x / 2, scrollCenter.y + scrollerSize.y / 2};

		// Create a Layout for the World Tiles
		const int rows = static_cast<int>(m_WorldTiles.size());
		const int worldTileHeight = static_cast<int>(round(144.f / 1009.f * s_ScreenHeight));
		const int worldTileWidth = static_cast<int>(round(1080.f / 1920.f * s_ScreenWidth));
		const glm::ivec2 worldTileSize{worldTileWidth, worldTileHeight};
		const int totalHeight = rows * worldTileHeight;

		m_Scroller.SetScrollAreaHeight(totalHeight);
		m_Scroller.SetTopLeftCorner(scrollerTopLeftCorner);
		m_Scroller.SetBottomRightCorner(scrollerBottomRightCorner);

		m_Scroller.Render();

		// ---- Start Cissoring for Scroller ----
		m_Scroller.StartCissoring();

		// ---- Render World Tiles ----
		const glm::ivec2 firstTilePos{scrollCenter.x,
									  scrollerTopLeftCorner.y + (worldTileSize.y / 2) + (worldTileSize.y / 20) -
										  m_Scroller.GetContentYOffset()};

		int drawnTileIndex = 0;
		std::string filterText = m_TextFieldFilter.GetText();
		for (size_t i = 0; i < m_WorldTiles.size(); i++)
		{
			WorldTile& worldTile = *m_WorldTiles[i];

			// Checks if the world tile should be rendered based on the filter text
			const WorldInfos worldInfos = worldTile.GetWorldInfos();
			const std::string name = worldInfos.Name;
			const std::string description = worldTile.FormatDescription();
			const std::string details = worldTile.FormatDetails();
			bool matchesFilter = name.find(filterText) != std::string::npos ||
				description.find(filterText) != std::string::npos || details.find(filterText) != std::string::npos;

			if (!filterText.empty() && !matchesFilter)
			{
				continue;
			}

			glm::ivec2 tilePosition =
				firstTilePos + glm::ivec2{0, static_cast<int>(drawnTileIndex * worldTileSize.y - drawnTileIndex)};
			Visibility visibility = m_Scroller.GetControlVisibleArea(tilePosition, worldTileSize);

			worldTile.SetPosition(tilePosition);
			worldTile.SetSize(worldTileSize);
			worldTile.SetVisibility(visibility);
			worldTile.Render();

			drawnTileIndex++;
		}

		// ---- Stop Cissoring for Scroller ----
		m_Scroller.StopCissoring();

		// Create Layouts for buttons
		glm::ivec2 tableSize{static_cast<int>(1232.f / 1920.f * s_ScreenWidth), s_ControlHeight};
		int horizontalSpacing = static_cast<int>(std::round(32.f / 1920.f * s_ScreenWidth));

		TableLayout layoutButtonsTop = LayoutHelper::CreateTableLayout(1, 2, tableSize, horizontalSpacing, 0);
		glm::ivec2 layoutButtonsTop_TopLeftCorner{s_CenterX - (tableSize.x / 2),
												  static_cast<int>((827.f - 23.f) / 1009.f * s_ScreenHeight)};

		TableLayout layoutButtonsBottom = LayoutHelper::CreateTableLayout(1, 4, tableSize, horizontalSpacing, 0);
		glm::ivec2 layoutButtonsBottom_TopLeftCorner{s_CenterX - (tableSize.x / 2),
													 static_cast<int>((923.f - 23.f) / 1009.f * s_ScreenHeight)};
		// ---- Render Play Selected World Button ----
		glm::ivec2 buttonPos = layoutButtonsTop_TopLeftCorner + layoutButtonsTop.GetElementPosition(0, 0);
		glm::ivec2 buttonSize = layoutButtonsTop.GetCellSize();
		bool isAnyWorldTileSelected = m_SelectedWorldIndex != -1;
		m_ButtonPlaySelectedWorld.SetEnabled(isAnyWorldTileSelected);
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
		m_ButtonDeleteSelectedWorld.SetEnabled(isAnyWorldTileSelected);
		m_ButtonDeleteSelectedWorld.SetPosition(buttonPos);
		m_ButtonDeleteSelectedWorld.SetSize(buttonSize);
		m_ButtonDeleteSelectedWorld.Render();

		// ---- Render Refresh World Tiles Button ----
		buttonPos = layoutButtonsBottom_TopLeftCorner + layoutButtonsBottom.GetElementPosition(0, 2);
		buttonSize = layoutButtonsBottom.GetCellSize();
		m_ButtonRefreshWorldTiles.SetPosition(buttonPos);
		m_ButtonRefreshWorldTiles.SetSize(buttonSize);
		m_ButtonRefreshWorldTiles.Render();

		// ---- Render Back Button ----
		buttonPos = layoutButtonsBottom_TopLeftCorner + layoutButtonsBottom.GetElementPosition(0, 3);
		buttonSize = layoutButtonsBottom.GetCellSize();
		m_ButtonBack.SetPosition(buttonPos);
		m_ButtonBack.SetSize(buttonSize);
		m_ButtonBack.Render();
	}

	void SingleplayerPanel::RenderDeleteConfirmation()
	{
		if (s_IsBackPressed || m_SelectedWorldIndex == -1)
		{
			m_CurrentRenderModule = eRenderModule::WorldTiles;
			return;
		}

		WorldInfos selectedWorldInfos = m_WorldTiles[m_SelectedWorldIndex]->GetWorldInfos();
		int centerX = s_ScreenWidth / 2;
		float textHeight = s_ScreenHeight * (28.f / 1009.f);

		// ---- Render Warning Text ----
		const std::string warningText = "Are you sure you want to delete this world?";
		float warningTextYOffsetRatio = (400.f - 23.f) / 1009.f;
		glm::ivec2 warningTextPos{centerX, static_cast<int>(s_ScreenHeight * warningTextYOffsetRatio)};
		m_LabelDeleteWarning.SetPosition(warningTextPos);
		m_LabelDeleteWarning.SetText(warningText);
		m_LabelDeleteWarning.SetTextHeight(textHeight);
		m_LabelDeleteWarning.Render();

		// ---- Render Details Text ----
		const std::string detailsText = "'" + selectedWorldInfos.Name + "' will be lost forever! (A long time!)";
		float detailsTextYOffsetRatio = (475.f - 23.f) / 1009.f;
		glm::ivec2 detailsTextPos{centerX, static_cast<int>(s_ScreenHeight * detailsTextYOffsetRatio)};
		m_LabelDeleteDetails.SetPosition(detailsTextPos);
		m_LabelDeleteDetails.SetText(detailsText);
		m_LabelDeleteDetails.SetTextHeight(textHeight);
		m_LabelDeleteDetails.Render();

		// Create Layout for buttons
		float tableWidthRatio = 1216.f / 1920.f;
		float tableHeightRatio = 82.f / 1009.f;
		float horizontalSpacingRatio = 20.f / 1920.f;
		int horizontalSpacing = static_cast<int>(std::round(horizontalSpacingRatio * s_ScreenWidth));
		glm::ivec2 tableSize{static_cast<int>(s_ScreenWidth * tableWidthRatio),
							 static_cast<int>(s_ScreenHeight * tableHeightRatio)};
		TableLayout layoutButtons = LayoutHelper::CreateTableLayout(1, 2, tableSize, horizontalSpacing, 0);

		float tableYOffsetRatio = 625.f / 1009.f;
		int tableY = static_cast<int>(s_ScreenHeight * tableYOffsetRatio);
		glm::ivec2 tableTopLeftCorner{centerX - (tableSize.x / 2), tableY - (tableSize.y / 2)};

		// ---- Render Delete Confirm Button ----
		glm::ivec2 buttonPos = tableTopLeftCorner + layoutButtons.GetElementPosition(0, 0);
		glm::ivec2 buttonSize = layoutButtons.GetCellSize();
		m_ButtonDeleteConfirm.SetPosition(buttonPos);
		m_ButtonDeleteConfirm.SetSize(buttonSize);
		m_ButtonDeleteConfirm.Render();

		// ---- Render Delete Cancel Button ----
		buttonPos = tableTopLeftCorner + layoutButtons.GetElementPosition(0, 1);
		buttonSize = layoutButtons.GetCellSize();
		m_ButtonDeleteCancel.SetPosition(buttonPos);
		m_ButtonDeleteCancel.SetSize(buttonSize);
		m_ButtonDeleteCancel.Render();
	}

	void SingleplayerPanel::RenderEditWorld() {}

	void SingleplayerPanel::RenderCreateNewWorld()
	{
		if (s_IsBackPressed)
		{
			m_CurrentRenderModule = eRenderModule::WorldTiles;
			return;
		}

		// Constants
		int centerX = static_cast<int>(std::round(s_ScreenWidth / 2.0));
		float controlsWidthRatio = 840.f / 1920.f;
		float controlsHeightRatio = 80.f / 1009.f;
		glm::ivec2 controlsSize{static_cast<int>(std::round(s_ScreenWidth * controlsWidthRatio)),
								static_cast<int>(std::round(s_ScreenHeight * controlsHeightRatio))};
		int textStartX = centerX - static_cast<int>(controlsSize.x / 2) + static_cast<int>(controlsSize.x * 0.05f);
		float normalTextHeight = std::round(s_ScreenHeight * 28.f / 1009.f);

		// ---- Render Title ----
		float titleYOffsetRatio = (120.f - 23.f) / 1009.f;
		float titleTextHeight = std::round(s_ScreenHeight * 38.f / 1009.f);
		glm::ivec2 titlePos{centerX, static_cast<int>(s_ScreenHeight * titleYOffsetRatio)};
		m_LabelCreateNewWorldTitle.SetPosition(titlePos);
		m_LabelCreateNewWorldTitle.SetTextHeight(titleTextHeight);
		m_LabelCreateNewWorldTitle.Render();

		// ---- Render Scroller ----
		float scrollerWidthRatio = 1.f;
		float scrollerHeightRatio = 580.f / 1009.f;
		glm::ivec2 scrollerSize{static_cast<int>(s_ScreenWidth * scrollerWidthRatio),
								static_cast<int>(s_ScreenHeight * scrollerHeightRatio)};
		float scrollCenterYratio = (503.f - 23.f) / 1009.f;
		glm::ivec2 scrollCenter{centerX, static_cast<int>(s_ScreenHeight * scrollCenterYratio)};

		glm::ivec2 scrollerTopLeftCorner{scrollCenter.x - scrollerSize.x / 2, scrollCenter.y - scrollerSize.y / 2};
		glm::ivec2 scrollerBottomRightCorner{scrollCenter.x + scrollerSize.x / 2, scrollCenter.y + scrollerSize.y / 2};

		m_ScrollerCreateNewWorld.SetTopLeftCorner(scrollerTopLeftCorner);
		m_ScrollerCreateNewWorld.SetBottomRightCorner(scrollerBottomRightCorner);

		m_ScrollerCreateNewWorld.Render();

		// ---- Render Label Create New World Name ----
		float nameLabelYOffsetRatio = (300.f - 23.f) / 1009.f;
		glm::ivec2 nameLabelPos{textStartX, static_cast<int>(s_ScreenHeight * nameLabelYOffsetRatio)};
		m_LabelCreateNewWorldName.SetPosition(nameLabelPos);
		m_LabelCreateNewWorldName.SetTextHeight(normalTextHeight);
		m_LabelCreateNewWorldName.Render();

		// ---- Render Text Field Create New World Name ----
		float nameTextFieldYOffsetRatio = nameLabelYOffsetRatio + (controlsHeightRatio / 1.3f);
		glm::ivec2 nameTextFieldPos{centerX, static_cast<int>(s_ScreenHeight * nameTextFieldYOffsetRatio)};
		m_TextFieldCreateNewWorldName.SetPosition(nameTextFieldPos);
		m_TextFieldCreateNewWorldName.SetSize(controlsSize);
		m_TextFieldCreateNewWorldName.Render();

		// ---- Render Button Create New World Select Type ----
		float selectTypeButtonYOffsetRatio = nameTextFieldYOffsetRatio + (controlsHeightRatio * 1.2f);
		glm::ivec2 selectTypeButtonPos{centerX, static_cast<int>(s_ScreenHeight * selectTypeButtonYOffsetRatio)};
		std::string text =
			"Type: " + WorldGenerator::WorldGenerationTypeToString(m_WorldInfosToCreate.WorldGenerationType);
		m_ButtonCreateNewWorldSelectType.SetText(text);
		m_ButtonCreateNewWorldSelectType.SetPosition(selectTypeButtonPos);
		m_ButtonCreateNewWorldSelectType.SetSize(controlsSize);
		m_ButtonCreateNewWorldSelectType.Render();

		// ---- Render Label Create New World Seed ----
		float seedLabelYOffsetRatio = selectTypeButtonYOffsetRatio + (controlsHeightRatio * 1.2f);
		glm::ivec2 seedLabelPos{textStartX, static_cast<int>(s_ScreenHeight * seedLabelYOffsetRatio)};
		m_LabelCreateNewWorldSeed.SetPosition(seedLabelPos);
		m_LabelCreateNewWorldSeed.SetTextHeight(normalTextHeight);
		m_LabelCreateNewWorldSeed.Render();

		// ---- Render Text Field Create New World Seed ----
		float seedTextFieldYOffsetRatio = seedLabelYOffsetRatio + (controlsHeightRatio / 1.3f);
		glm::ivec2 seedTextFieldPos{centerX, static_cast<int>(s_ScreenHeight * seedTextFieldYOffsetRatio)};
		m_TextFieldCreateNewWorldSeed.SetPosition(seedTextFieldPos);
		m_TextFieldCreateNewWorldSeed.SetSize(controlsSize);
		m_TextFieldCreateNewWorldSeed.Render();

		// Create Layout for buttons
		float tableWidthRatio = 1230.f / 1920.f;
		float tableHeightRatio = 80.f / 1009.f;
		int horizontalSpacing = static_cast<int>(std::round(30.f / 1920.f * s_ScreenWidth));
		glm::ivec2 tableSize{static_cast<int>(s_ScreenWidth * tableWidthRatio),
							 static_cast<int>(s_ScreenHeight * tableHeightRatio)};
		TableLayout layoutButtons = LayoutHelper::CreateTableLayout(1, 2, tableSize, horizontalSpacing, 0);
		float tableYOffsetRatio = (915.f - 23.f) / 1009.f;
		glm::ivec2 tableTopLeftCorner{centerX - (tableSize.x / 2),
									  static_cast<int>(s_ScreenHeight * tableYOffsetRatio) - (tableSize.y / 2)};

		// ---- Render Button Create New World Confirm ----
		glm::ivec2 buttonPos = tableTopLeftCorner + layoutButtons.GetElementPosition(0, 0);
		m_ButtonCreateNewWorldConfirm.SetPosition(buttonPos);
		m_ButtonCreateNewWorldConfirm.SetSize(layoutButtons.GetCellSize());
		m_ButtonCreateNewWorldConfirm.Render();

		// ---- Render Button Create New World Cancel ----
		buttonPos = tableTopLeftCorner + layoutButtons.GetElementPosition(0, 1);
		m_ButtonCreateNewWorldCancel.SetPosition(buttonPos);
		m_ButtonCreateNewWorldCancel.SetSize(layoutButtons.GetCellSize());
		m_ButtonCreateNewWorldCancel.Render();
	}

	void SingleplayerPanel::ClearWorldTiles()
	{
		for (auto& worldTile : m_WorldTiles)
		{
			worldTile->Delete();
		}

		m_WorldTiles.clear();

		m_SelectedWorldIndex = -1;
	}

	std::filesystem::path SingleplayerPanel::GetSavesDirectoryPath() const
	{
		return Utils::GetExecutableDirectory() / s_SavesDirectory;
	}

	std::vector<WorldInfos> SingleplayerPanel::GetWorldsInfos() const
	{

		std::vector<WorldInfos> worldsInfos;
		std::filesystem::path savesDirectoryPath = GetSavesDirectoryPath();

		if (!std::filesystem::exists(savesDirectoryPath) || !std::filesystem::is_directory(savesDirectoryPath))
		{
			return worldsInfos;
		}

		WorldInfos worldInfos;
		for (const auto& entry : std::filesystem::directory_iterator(savesDirectoryPath))
		{
			if (entry.is_directory())
			{
				std::filesystem::path worldSavePath = entry.path();

				bool success = WorldSave::GetWorldInfos(worldSavePath, worldInfos);

				if (success)
				{
					worldsInfos.push_back(std::move(worldInfos));
				}
			}
		}

		return worldsInfos;
	}

	void SingleplayerPanel::SubscribeToControlEvents()
	{
		m_EventHandles.push_back(
			m_ButtonBack.EvtClick.Subscribe([this](const Button& button) { Handle_ButtonBackClick(button); }));

		m_EventHandles.push_back(m_ButtonCreateNewWorld.EvtClick.Subscribe(
			[this](const Button& button) { Handle_ButtonCreateNewWorldClick(button); }));

		m_EventHandles.push_back(m_ButtonPlaySelectedWorld.EvtClick.Subscribe(
			[this](const Button& button) { Handle_PlaySelectedWorldClick(button); }));

		m_EventHandles.push_back(m_ButtonDeleteSelectedWorld.EvtClick.Subscribe(
			[this](const Button& button) { Handle_ButtonDeleteSelectedWorldClick(button); }));

		m_EventHandles.push_back(
			m_ButtonEdit.EvtClick.Subscribe([this](const Button& button) { Handle_ButtonEditClick(button); }));

		m_EventHandles.push_back(m_ButtonRefreshWorldTiles.EvtClick.Subscribe(
			[this](const Button& button) { Handle_ButtonRefreshWorldTilesClick(button); }));

		m_EventHandles.push_back(m_ButtonDeleteConfirm.EvtClick.Subscribe([this](const Button& button)
																		 { Handle_DeleteConfirmClick(button); }));

		m_EventHandles.push_back(
			m_ButtonDeleteCancel.EvtClick.Subscribe([this](const Button& button) { Handle_DeleteCancelClick(button); }));

		m_EventHandles.push_back(m_ButtonCreateNewWorldSelectType.EvtClick.Subscribe(
			[this](const Button& button) { Handle_CreateNewWorldSelectTypeClick(button); }));

		m_EventHandles.push_back(m_ButtonCreateNewWorldConfirm.EvtClick.Subscribe(
			[this](const Button& button) { Handle_CreateNewWorldConfirmClick(button); }));

		m_EventHandles.push_back(m_ButtonCreateNewWorldCancel.EvtClick.Subscribe(
			[this](const Button& button) { Handle_CreateNewWorldCancelClick(button); }));
	}

	void SingleplayerPanel::Handle_ButtonBackClick(const Button& button)
	{
		(void) button;
		m_SelectedWorldIndex = -1;

		// Unselect all world tiles
		for (const auto& worldTile : m_WorldTiles)
		{
			worldTile->SetSelected(false);
		}

		// Clear Filter Text
		m_TextFieldFilter.SetText("");

		EvtRequestBackNavigation.Trigger(this);
	}

	void SingleplayerPanel::Handle_ButtonCreateNewWorldClick(const Button& button)
	{
		(void) button;

		m_WorldInfosToCreate = WorldInfos();
		m_TextFieldCreateNewWorldName.SetText("");
		m_TextFieldCreateNewWorldSeed.SetText("");
		m_CurrentRenderModule = eRenderModule::CreateNewWorld;
	}

	void SingleplayerPanel::Handle_PlaySelectedWorldClick(const Button& button)
	{
		(void) button;

		WorldInfos worldInfos = m_WorldTiles[m_SelectedWorldIndex]->GetWorldInfos();

		std::cout << "Play world: '" << worldInfos.Name << "', Directory: '" << worldInfos.SaveDirectory << "'"
				  << std::endl;

		EvtPlayWorld.Trigger(worldInfos);
	}

	void SingleplayerPanel::Handle_ButtonDeleteSelectedWorldClick(const Button& button)
	{
		(void) button;

		m_CurrentRenderModule = eRenderModule::DeleteConfirmation;
	}

	void SingleplayerPanel::Handle_ButtonEditClick(const Button& button)
	{
		(void) button;
		assert(false && "Not implemented yet");
	}

	void SingleplayerPanel::Handle_ButtonRefreshWorldTilesClick(const Button& button)
	{
		(void) button;

		RefreshWorldTiles();
	}

	void SingleplayerPanel::Handle_WorldTileSelected(const WorldTile& worldTile)
	{
		// Deselect all other tiles
		for (int i = 0; i < m_WorldTiles.size(); i++)
		{
			if (m_WorldTiles[i].get() != &worldTile)
			{
				m_WorldTiles[i]->SetSelected(false);
			}
			else
			{
				m_SelectedWorldIndex = i;
			}
		}
	}

	void SingleplayerPanel::Handle_WorldTileDoubleClicked(const WorldTile& worldTile)
	{
		Handle_WorldTileSelected(worldTile);
		Handle_PlaySelectedWorldClick(m_ButtonPlaySelectedWorld);
	}

	void SingleplayerPanel::Handle_DeleteConfirmClick(const Button& button)
	{
		(void) button;

		if (m_SelectedWorldIndex == -1)
		{
			return;
		}

		WorldInfos worldInfos = m_WorldTiles[m_SelectedWorldIndex]->GetWorldInfos();

		WorldSave::DeleteWorld(worldInfos);

		RefreshWorldTiles();

		m_CurrentRenderModule = eRenderModule::WorldTiles;
	}

	void SingleplayerPanel::Handle_DeleteCancelClick(const Button& button)
	{
		(void) button;

		m_CurrentRenderModule = eRenderModule::WorldTiles;
	}

	void SingleplayerPanel::Handle_CreateNewWorldSelectTypeClick(const Button& button)
	{
		(void) button;

		// Round Robin through the world generation types

		WorldGenerator::eWorldGenerationType currentType = m_WorldInfosToCreate.WorldGenerationType;

		currentType = static_cast<WorldGenerator::eWorldGenerationType>(
			(static_cast<uint8_t>(currentType) + 1) %
			static_cast<uint8_t>(WorldGenerator::eWorldGenerationType::Count));

		m_WorldInfosToCreate.WorldGenerationType = currentType;
	}

	void SingleplayerPanel::Handle_CreateNewWorldConfirmClick(const Button& button)
	{
		(void) button;

		// Validate world name
		std::string worldName = m_TextFieldCreateNewWorldName.GetText();
		m_WorldInfosToCreate.Name = worldName;
		if (m_WorldInfosToCreate.Name.empty())
		{
			m_WorldInfosToCreate.Name = "New World";
		}

		// Validate Seed
		std::string seedText = m_TextFieldCreateNewWorldSeed.GetText();
		if (!seedText.empty())
		{
			try
			{
				m_WorldInfosToCreate.Seed = static_cast<uint32_t>(std::stoul(seedText));
			}
			catch (const std::exception&)
			{
				m_WorldInfosToCreate.Seed = 0;
			}
		}
		else
		{
			m_WorldInfosToCreate.Seed = 0;
		}

		std::string saveDirectoryName = Utils::SanitizeFileName(worldName);

		auto saveDirectory = GetSavesDirectoryPath() / saveDirectoryName;
		WorldSave::CreateWorld(saveDirectory, m_WorldInfosToCreate);

		RefreshWorldTiles();

		// Retreve the created WorldInfos with the correct SaveDirectory
		WorldInfos worldInfos;
		if (WorldSave::GetWorldInfos(saveDirectory, worldInfos))
		{
			std::cout << "Created new world: '" << worldInfos.Name << "', Directory: '" << worldInfos.SaveDirectory
					  << "'" << std::endl;

			// Go back to world tiles render module for next navigation of SingleplayerPanel.
			m_CurrentRenderModule = eRenderModule::WorldTiles;

			// Start Playing the newly created world
			EvtPlayWorld.Trigger(worldInfos);
		}
		else
		{
			std::cerr << "Error: Failed to retrieve WorldInfos for the newly created world at '" << saveDirectory << "'"
					  << std::endl;

			Handle_CreateNewWorldCancelClick(m_ButtonCreateNewWorldCancel);
		}
	}

	void SingleplayerPanel::Handle_CreateNewWorldCancelClick(const Button& button)
	{
		(void) button;
		m_WorldInfosToCreate = WorldInfos();
		m_TextFieldCreateNewWorldName.SetText("");
		m_TextFieldCreateNewWorldSeed.SetText("");
		m_CurrentRenderModule = eRenderModule::WorldTiles;
	}

} // namespace onion::voxel
