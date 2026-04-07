#include "ResourcePacksPanel.hpp"

#include <nlohmann/json.hpp>

#include <unordered_set>

#include <shared/zip_archive/ZipArchive.hpp>

#include <renderer/gui/LayoutHelper.hpp>

namespace onion::voxel
{
	ResourcePacksPanel::ResourcePacksPanel(const std::string& name)
		: GuiElement(name), m_Title_Label("ResourcePacksTitle_Label"),
		  m_Description_Label("ResourcePacksDescription_Label"), m_OpenPackFolder_Button("OpenPackFolder_Button"),
		  m_Done_Button("Done_Button"), m_DefaultResourcePack_Tile("DefaultResourcePack_Tile"),
		  m_Search_TextField("Search_TextField"), m_ResourcePacks_Scroller("ResourcePacks_Scroller")
	{
		SubscribeToControlEvents();

		m_TimerScanResourcePacksFolder.setElapsedPeriod(std::chrono::seconds(1));
		m_TimerScanResourcePacksFolder.setTimeoutFunction([this]() { ScanResourcePacksFolder(); });

		m_Title_Label.SetText("Select Resource Packs");
		m_Title_Label.SetTextAlignment(Font::eTextAlignment::Center);

		m_Description_Label.SetText("Place .zip texture packs in the folder. Open with the button below.");
		m_Description_Label.SetTextAlignment(Font::eTextAlignment::Center);
		m_Description_Label.SetTextColor(s_ColorSecondaryText);

		m_Search_TextField.SetPlaceholderText("Search...");

		m_DefaultResourcePack_Tile.SetResourcePackName("Default");
		m_DefaultResourcePack_Tile.SetResourcePackDescription("The default look of Onion::Voxel (built-in)");

		m_OpenPackFolder_Button.SetText("Open Pack Folder");

		m_Done_Button.SetText("Done");

		SetCurrentlySelectedResourcePack("Default");
	}

	ResourcePacksPanel::~ResourcePacksPanel()
	{
		m_EventHandles.clear();
	}

	void ResourcePacksPanel::Render()
	{
		// Delete any resource pack infos that need to be deleted
		std::unique_ptr<ResourcePackTile> tileToDelete;
		while (m_ResourcePackTilesToDelete.TryPop(tileToDelete))
		{
			tileToDelete->Delete();
		}

		if (s_IsBackPressed)
		{
			m_TimerScanResourcePacksFolder.Stop();
			RequestBackNavigation.Trigger(this);
			return;
		}

		// ---- Constants for Layout ----
		glm::vec2 buttonSizeRatio{0.415f, 0.08f};
		glm::vec2 buttonSize{buttonSizeRatio.x * s_ScreenWidth, buttonSizeRatio.y * s_ScreenHeight};
		float middleX = s_ScreenWidth * 0.5f;

		float firstTileYratio = (300.f - 23.f) / 1009.f;
		float currentTileY = s_ScreenHeight * firstTileYratio;
		float tileHeightRatio = 128.f / 1009.f;
		int tileHeight = static_cast<int>(round(s_ScreenHeight * tileHeightRatio));
		float tileWidthRatio = 0.6f;
		int tileWidth = static_cast<int>(round(s_ScreenWidth * tileWidthRatio));
		glm::vec2 tileSize{tileWidth, tileHeight};
		float tileSpacingYratio = 26.f / 1009.f;
		int tileSpacingY = static_cast<int>(round(s_ScreenHeight * tileSpacingYratio));

		// ---- Render Menu Title ----
		constexpr float titleYOffsetRatio = (57.f - 23.f) / 1009.f;
		glm::vec2 textTitlePosition = {middleX, s_ScreenHeight * titleYOffsetRatio};

		m_Title_Label.SetPosition(textTitlePosition);
		m_Title_Label.SetTextHeight(s_TextHeight);
		m_Title_Label.Render();

		// ---- Render Description Label ----
		constexpr float descYOffsetRatio = (109.f - 23.f) / 1009.f;
		glm::vec2 textDescPosition = {middleX, s_ScreenHeight * descYOffsetRatio};

		m_Description_Label.SetPosition(textDescPosition);
		m_Description_Label.SetTextHeight(s_TextHeight);
		m_Description_Label.Render();

		// ---- Render Search TextField ----
		constexpr float searchYOffsetRatio = (170.f - 23.f) / 1009.f;
		glm::vec2 searchTextFieldPosition = {middleX, s_ScreenHeight * searchYOffsetRatio};
		float searchTextFieldWidthRatio = 800.f / 1920.f;
		int searchTextFieldWidth = static_cast<int>(round(s_ScreenWidth * searchTextFieldWidthRatio));
		float searchTextFieldHeightRatio = 64.f / 1009.f;
		int searchTextFieldHeight = static_cast<int>(round(s_ScreenHeight * searchTextFieldHeightRatio));
		glm::ivec2 searchTextFieldSize{searchTextFieldWidth, searchTextFieldHeight};

		m_Search_TextField.SetPosition(searchTextFieldPosition);
		m_Search_TextField.SetSize(searchTextFieldSize);
		m_Search_TextField.Render();

		// ---- Render Scroller ----
		float scrollerYOffsetRatio = (558.f - 23.f) / 1009.f;
		int scrollerYcenter = static_cast<int>(round(s_ScreenHeight * scrollerYOffsetRatio));
		float scrollerHeightRatio = 690.f / 1009.f;
		int scrollerHeight = static_cast<int>(round(s_ScreenHeight * scrollerHeightRatio));
		int scrollerTopLeftY = scrollerYcenter - (scrollerHeight / 2);
		glm::ivec2 scrollerTopLeft{0, scrollerTopLeftY};
		int scrollerBottomY = scrollerYcenter + (scrollerHeight / 2);
		glm::ivec2 scrollerBottomRight{s_ScreenWidth, scrollerBottomY};

		m_ResourcePacks_Scroller.SetTopLeftCorner(scrollerTopLeft);
		m_ResourcePacks_Scroller.SetBottomRightCorner(scrollerBottomRight);
		// Compute Total Scroller Height
		{
			std::lock_guard lock(m_MutexResourcePacks);
			size_t totalTiles = m_ResourcePacksTiles.size() + 1; // +1 for the default pack
			int totalHeight = static_cast<int>(totalTiles * (tileHeight + tileSpacingY)) +
				static_cast<int>((currentTileY - tileHeight / 2) - scrollerTopLeftY);
			m_ResourcePacks_Scroller.SetScrollAreaHeight(totalHeight);
		}
		m_ResourcePacks_Scroller.Render();

		// ---- Start Cissoring for Resource Pack Tiles ----
		m_ResourcePacks_Scroller.StartCissoring();

		// ---- Render Resource Pack Tiles ----
		{
			std::lock_guard lock(m_MutexResourcePacks);

			std::vector<ResourcePackTile*> visibleTiles;
			visibleTiles.push_back(&m_DefaultResourcePack_Tile);
			for (const auto& resourcePackTile : m_ResourcePacksTiles)
			{
				visibleTiles.push_back(resourcePackTile.get());
			}

			std::string searchText = m_Search_TextField.GetText();
			if (!searchText.empty())
			{
				visibleTiles.erase(std::remove_if(visibleTiles.begin(),
												  visibleTiles.end(),
												  [&searchText](ResourcePackTile* tile)
												  {
													  std::string name = tile->GetResourcePackName();
													  std::string description = tile->GetResourcePackDescription();
													  return name.find(searchText) == std::string::npos &&
														  description.find(searchText) == std::string::npos;
												  }),
								   visibleTiles.end());
			}

			for (const auto& resourcePackTile : visibleTiles)
			{
				glm::ivec2 tilePosition = {middleX, currentTileY - m_ResourcePacks_Scroller.GetContentYOffset()};
				Visibility tileVisibility = m_ResourcePacks_Scroller.GetControlVisibleArea(tilePosition, tileSize);
				resourcePackTile->SetPosition(tilePosition);
				resourcePackTile->SetSize(tileSize);
				resourcePackTile->SetVisibility(tileVisibility);
				resourcePackTile->Render();
				currentTileY += tileSize.y + tileSpacingY; // Add some spacing between tiles
			}
		}

		// ---- Stop Cissoring for Resource Pack Tiles ----
		m_ResourcePacks_Scroller.StopCissoring();

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
			1, 2, glm::ivec2(tablesWidth, table1Height), (int) horizontalSpacings, (int) verticalSpacings);

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
		m_Description_Label.Initialize();
		m_OpenPackFolder_Button.Initialize();
		m_Done_Button.Initialize();
		m_DefaultResourcePack_Tile.Initialize();
		m_Search_TextField.Initialize();
		m_ResourcePacks_Scroller.Initialize();

		SetInitState(true);
	}

	void ResourcePacksPanel::Delete()
	{
		m_Title_Label.Delete();
		m_Description_Label.Delete();
		m_OpenPackFolder_Button.Delete();
		m_Done_Button.Delete();
		m_DefaultResourcePack_Tile.Delete();
		m_Search_TextField.Delete();
		m_ResourcePacks_Scroller.Delete();

		{
			std::lock_guard lock(m_MutexResourcePacks);
			for (auto& resourcePackTile : m_ResourcePacksTiles)
			{
				resourcePackTile->Delete();
			}
			m_ResourcePacksTiles.clear();
		}

		SetDeletedState(true);
	}

	void ResourcePacksPanel::ReloadTextures()
	{
		m_Title_Label.ReloadTextures();
		m_Description_Label.ReloadTextures();
		m_OpenPackFolder_Button.ReloadTextures();
		m_Done_Button.ReloadTextures();
		m_DefaultResourcePack_Tile.ReloadTextures();
		m_Search_TextField.ReloadTextures();
		m_ResourcePacks_Scroller.ReloadTextures();

		std::lock_guard lock(m_MutexResourcePacks);
		for (auto& resourcePackTile : m_ResourcePacksTiles)
		{
			resourcePackTile->ReloadTextures();
		}
	}

	void ResourcePacksPanel::ScanResourcePacksFolder()
	{
		m_TimerScanResourcePacksFolder.Start();

		const std::filesystem::path resourcePacksDirectory = EngineContext::Get().Assets->GetResourcePacksDirectory();

		std::unordered_set<std::string> packsOnDisk;

		for (const auto& entry : std::filesystem::directory_iterator(resourcePacksDirectory))
		{
			if (!entry.is_regular_file() || entry.path().extension() != ".zip")
				continue;

			std::string resourcePackName = entry.path().stem().string();

			if (resourcePackName == "Default")
				continue;

			packsOnDisk.insert(resourcePackName);

			if (ContainsResourcePack(resourcePackName))
				continue;

			ZipArchive zip(entry.path());

			std::string packMcmeta = zip.GetFileText("pack.mcmeta");

			nlohmann::json mcmetaJson = nlohmann::json::parse(packMcmeta);
			std::string packDescription = mcmetaJson["pack"]["description"].get<std::string>();

			std::string thumbnailName = resourcePackName + ".pack.png";
			std::vector<unsigned char> thumbnailData = zip.GetFileBinary("pack.png");
			Texture thumbnail(thumbnailName, thumbnailData);

			std::unique_ptr<ResourcePackTile> resourcePackTile =
				std::make_unique<ResourcePackTile>(resourcePackName, std::move(thumbnail));

			resourcePackTile->SetResourcePackName(resourcePackName);
			resourcePackTile->SetResourcePackDescription(packDescription);

			// Subscribe to the tile's checked changed event
			m_EventHandles.push_back(resourcePackTile->EvtCheckedChanged.Subscribe(
				[this](const ResourcePackTile& tile) { Handle_ResourcePackTileCheckedChanged(tile); }));

			{
				std::lock_guard lock(m_MutexResourcePacks);
				m_ResourcePacksTiles.push_back(std::move(resourcePackTile));
			}
		}

		// Remove packs that no longer exist on disk
		{
			std::lock_guard lock(m_MutexResourcePacks);

			auto it = m_ResourcePacksTiles.begin();
			while (it != m_ResourcePacksTiles.end())
			{
				if (!packsOnDisk.contains((*it)->GetResourcePackName()))
				{
					// Move the pack to the deletion queue
					m_ResourcePackTilesToDelete.Push(std::move(*it));

					// Remove from vector
					it = m_ResourcePacksTiles.erase(it);
				}
				else
				{
					++it;
				}
			}
		}
	}

	void ResourcePacksPanel::SetCurrentlySelectedResourcePack(const std::string& resourcePackName)
	{
		if (resourcePackName == "Default")
		{
			m_DefaultResourcePack_Tile.SetSelected(true);
		}

		// Uncheck all other tiles and check the one that matches the selected resource pack
		{
			bool selectedDefaultPack = resourcePackName == m_DefaultResourcePack_Tile.GetResourcePackName();
			m_DefaultResourcePack_Tile.SetSelected(selectedDefaultPack);

			std::lock_guard lock(m_MutexResourcePacks);
			for (const auto& resourcePackTile : m_ResourcePacksTiles)
			{
				bool isSelectedTile = resourcePackTile->GetResourcePackName() == resourcePackName;
				resourcePackTile->SetSelected(isSelectedTile);
			}
		}

		m_CurrentlySelectedResourcePackName = resourcePackName;
	}

	std::string ResourcePacksPanel::GetCurrentlySelectedResourcePack() const
	{
		std::lock_guard lock(m_MutexResourcePacks);
		return m_CurrentlySelectedResourcePackName;
	}

	void ResourcePacksPanel::SubscribeToControlEvents()
	{
		m_EventHandles.push_back(m_OpenPackFolder_Button.OnClick.Subscribe([this](const Button& sender)
																		   { Handle_OpenPackFolder_Click(sender); }));

		m_EventHandles.push_back(
			m_Done_Button.OnClick.Subscribe([this](const Button& sender) { Handle_Done_Click(sender); }));

		m_EventHandles.push_back(m_DefaultResourcePack_Tile.EvtCheckedChanged.Subscribe(
			[this](const ResourcePackTile& tile) { Handle_ResourcePackTileCheckedChanged(tile); }));
	}

	void ResourcePacksPanel::Handle_ResourcePackTileCheckedChanged(const ResourcePackTile& tile)
	{
		if (tile.IsSelected())
		{
			SetCurrentlySelectedResourcePack(tile.GetResourcePackName());
		}
		else
		{
			// Checks if any tile is still checked
			bool anyTileChecked = false;
			{
				std::lock_guard lock(m_MutexResourcePacks);
				for (const auto& resourcePackTile : m_ResourcePacksTiles)
				{
					if (resourcePackTile->IsSelected())
					{
						anyTileChecked = true;
						break;
					}
				}
			}

			// If no tiles are checked, select the default resource pack
			if (!anyTileChecked)
			{
				SetCurrentlySelectedResourcePack(m_DefaultResourcePack_Tile.GetResourcePackName());
			}
		}
	}

	void ResourcePacksPanel::Handle_OpenPackFolder_Click(const Button& sender)
	{
		(void) sender; // Unused parameter)
		std::filesystem::path resourcePacksFolderPath = EngineContext::Get().Assets->GetResourcePacksDirectory();
		ShellExecuteA(
			nullptr, "open", "explorer.exe", resourcePacksFolderPath.string().c_str(), nullptr, SW_SHOWNORMAL);
	}

	void ResourcePacksPanel::Handle_Done_Click(const Button& sender)
	{
		(void) sender; // Unused parameter)

		m_TimerScanResourcePacksFolder.Stop();

		std::string currentPack = EngineContext::Get().Assets->GetCurrentResourcePack();
		std::string selectedPack = GetCurrentlySelectedResourcePack();

		if (currentPack != selectedPack)
		{
			auto settings = EngineContext::Get().Settings();
			settings.ResourcePack = selectedPack;

			UserSettingsChangedEventArgs args(settings);
			args.ResourcePack_Changed = true;

			EvtUserSettingsChanged.Trigger(args);
		}

		RequestBackNavigation.Trigger(this);
	}

	bool ResourcePacksPanel::ContainsResourcePack(const std::string& resourcePackName) const
	{
		std::lock_guard lock(m_MutexResourcePacks);
		for (const auto& resourcePack : m_ResourcePacksTiles)
		{
			if (resourcePack->GetResourcePackName() == resourcePackName)
			{
				return true;
			}
		}
		return false;
	}

} // namespace onion::voxel
