#include "SingleplayerPanel.hpp"

#include <shared/utils/Utils.hpp>

#include <renderer/gui/LayoutHelper.hpp>

namespace onion::voxel
{
	SingleplayerPanel::SingleplayerPanel(const std::string& name)
		: GuiElement(name), m_LabelTitle("Title"), m_TextFieldFilter("Filter"), m_Scroller("Scroller"),
		  m_ButtonBack("Back"), m_ButtonCreateNewWorld("Create New World"),
		  m_ButtonPlaySelectedWorld("Play Selected World"), m_ButtonDeleteSelectedWorld("Delete Selected World"),
		  m_ButtonEdit("Edit"), m_ButtonRefreshWorldTiles("Re-Create Selected World")
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

		m_ButtonRefreshWorldTiles.SetText("Refresh");

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
			Handle_ButtonBackClick(m_ButtonBack);
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
		const int rows = m_WorldTiles.size();
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
									  scrollerTopLeftCorner.y + (worldTileSize.y / 2) + (worldTileSize.y / 10) -
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

			worldTile.SetPosition(firstTilePos + glm::ivec2{0, static_cast<int>(drawnTileIndex * worldTileSize.y)});
			worldTile.SetSize(worldTileSize);
			worldTile.Render();

			drawnTileIndex++;
		}

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
		bool isPlayButtonEnabled = m_SelectedWorldIndex != -1;
		m_ButtonPlaySelectedWorld.SetEnabled(isPlayButtonEnabled);
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
			m_ButtonBack.OnClick.Subscribe([this](const Button& button) { Handle_ButtonBackClick(button); }));

		m_EventHandles.push_back(m_ButtonCreateNewWorld.OnClick.Subscribe(
			[this](const Button& button) { Handle_ButtonCreateNewWorldClick(button); }));

		m_EventHandles.push_back(m_ButtonPlaySelectedWorld.OnClick.Subscribe(
			[this](const Button& button) { Handle_PlaySelectedWorldClick(button); }));

		m_EventHandles.push_back(m_ButtonDeleteSelectedWorld.OnClick.Subscribe(
			[this](const Button& button) { Handle_ButtonDeleteSelectedWorldClick(button); }));

		m_EventHandles.push_back(
			m_ButtonEdit.OnClick.Subscribe([this](const Button& button) { Handle_ButtonEditClick(button); }));

		m_EventHandles.push_back(m_ButtonRefreshWorldTiles.OnClick.Subscribe(
			[this](const Button& button) { Handle_ButtonRefreshWorldTilesClick(button); }));
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

		WorldInfos worldInfos = m_WorldTiles[m_SelectedWorldIndex]->GetWorldInfos();

		std::cout << "Play world: '" << worldInfos.Name << "', Directory: '" << worldInfos.SaveDirectory << "'"
				  << std::endl;

		EvtPlayWorld.Trigger(worldInfos);
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

} // namespace onion::voxel
