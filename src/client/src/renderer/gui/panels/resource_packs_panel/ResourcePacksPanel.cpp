#include "ResourcePacksPanel.hpp"

#include <nlohmann/json.hpp>

#include <unordered_set>

#include <shared/zip_archive/ZipArchive.hpp>

#include "../../../Variables.hpp"
#include "../../LayoutHelper.hpp"

#include <windows.h>

namespace onion::voxel
{
	ResourcePacksPanel::ResourcePacksPanel(const std::string& name)
		: GuiElement(name), m_Title_Label("ResourcePacksTitle_Label"),
		  m_Description_Label("ResourcePacksDescription_Label"), m_OpenPackFolder_Button("OpenPackFolder_Button"),
		  m_Done_Button("Done_Button"),
		  m_DefaultResourcePack_Tile("DefaultResourcePack_Tile", m_DefaultResourcePackThumbnailPath)
	{
		SubscribeToControlEvents();

		m_TimerScanResourcePacksFolder.setElapsedPeriod(std::chrono::seconds(1));
		m_TimerScanResourcePacksFolder.setTimeoutFunction([this]() { ScanResourcePacksFolder(); });

		m_Title_Label.SetText("Select Resource Packs");
		m_Title_Label.SetTextAlignment(Font::eTextAlignment::Center);

		m_Description_Label.SetText("Place .zip texture packs in the folder. Open with the button below.");
		m_Description_Label.SetTextAlignment(Font::eTextAlignment::Center);
		m_Description_Label.SetTextColor(s_ColorSecondaryText);

		m_DefaultResourcePack_Tile.SetResourcePackName("Default");
		m_DefaultResourcePack_Tile.SetResourcePackDescription("The default look and feel of Onion::Voxel (built-in)");

		m_OpenPackFolder_Button.SetText("Open Pack Folder");

		m_Done_Button.SetText("Done");

		SetCurrentlySelectedResourcePack(SelectedResourcePackName);
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
		float buttonYSpacingRatio = 94.f / 1009.f;
		float firstButtonYPosRatio = 486.f / 1009.f;
		float middleX = s_ScreenWidth * 0.5f;

		// ---- Render Menu Title ----
		constexpr float titleYOffsetRatio = 0.0320f;
		glm::vec2 textTitlePosition = {s_ScreenWidth / 2, s_ScreenHeight * titleYOffsetRatio};
		float textHeight = s_ScreenHeight * (31.f / 1009.f);

		m_Title_Label.SetPosition(textTitlePosition);
		m_Title_Label.SetTextHeight(textHeight);
		m_Title_Label.Render();

		// ---- Render Description Label ----
		constexpr float descYOffsetRatio = 0.0833f;
		glm::vec2 textDescPosition = {s_ScreenWidth / 2, s_ScreenHeight * descYOffsetRatio};

		m_Description_Label.SetPosition(textDescPosition);
		m_Description_Label.SetTextHeight(textHeight);
		m_Description_Label.Render();

		// ---- Render Resource Pack Tiles ----
		{
			std::lock_guard lock(m_MutexResourcePacks);
			float firstYratio = 0.25f;
			float currentY = s_ScreenHeight * firstYratio;
			float tileHeightRatio = 0.13f;
			float tileWidthRatio = 0.6f;
			glm::vec2 tileSize{s_ScreenWidth * tileWidthRatio, s_ScreenHeight * tileHeightRatio};
			float spacingYratio = 1.2f;

			// Render Default Resource Pack Tile
			m_DefaultResourcePack_Tile.SetPosition({middleX, currentY});
			m_DefaultResourcePack_Tile.SetSize(tileSize);
			m_DefaultResourcePack_Tile.Render();
			currentY += tileSize.y * spacingYratio; // Add some spacing after the default pack

			for (const auto& resourcePackTile : m_ResourcePacksTiles)
			{
				resourcePackTile->SetPosition({middleX, currentY});
				resourcePackTile->SetSize(tileSize);
				resourcePackTile->Render();
				currentY += tileSize.y * spacingYratio; // Add some spacing between tiles
			}
		}

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
		m_Description_Label.Initialize();
		m_OpenPackFolder_Button.Initialize();
		m_Done_Button.Initialize();
		m_DefaultResourcePack_Tile.Initialize();

		SetInitState(true);
	}

	void ResourcePacksPanel::Delete()
	{
		m_Title_Label.Delete();
		m_Description_Label.Delete();
		m_OpenPackFolder_Button.Delete();
		m_Done_Button.Delete();
		m_DefaultResourcePack_Tile.Delete();

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

	void ResourcePacksPanel::ScanResourcePacksFolder()
	{
		m_TimerScanResourcePacksFolder.Start();

		const std::filesystem::path resourcePacksDirectory = GetResourcePacksDirectory();

		std::unordered_set<std::string> packsOnDisk;

		for (const auto& entry : std::filesystem::directory_iterator(resourcePacksDirectory))
		{
			if (!entry.is_regular_file() || entry.path().extension() != ".zip")
				continue;

			std::string resourcePackName = entry.path().stem().string();
			packsOnDisk.insert(resourcePackName);

			if (ContainsResourcePack(resourcePackName))
				continue;

			ZipArchive zip(entry.path());

			std::string packMcmeta = zip.GetFileText("pack.mcmeta");

			nlohmann::json mcmetaJson = nlohmann::json::parse(packMcmeta);
			std::string packDescription = mcmetaJson["pack"]["description"].get<std::string>();

			std::string thumbnailName = resourcePackName + ".pack.png";
			std::vector<unsigned char> thumbnailData = zip.GetFileData("pack.png");
			Texture thumbnail(thumbnailName, thumbnailData);

			std::unique_ptr<ResourcePackTile> resourcePackTile =
				std::make_unique<ResourcePackTile>(resourcePackName, std::move(thumbnail));

			resourcePackTile->SetResourcePackName(resourcePackName);
			resourcePackTile->SetResourcePackDescription(packDescription);

			// Subscribe to the tile's checked changed event
			m_EventHandles.push_back(resourcePackTile->OnCheckedChanged.Subscribe(
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
			m_DefaultResourcePack_Tile.SetChecked(true);
		}

		// Uncheck all other tiles and check the one that matches the selected resource pack
		{
			bool selectedDefaultPack = resourcePackName == m_DefaultResourcePack_Tile.GetResourcePackName();
			m_DefaultResourcePack_Tile.SetChecked(selectedDefaultPack);

			std::lock_guard lock(m_MutexResourcePacks);
			for (const auto& resourcePackTile : m_ResourcePacksTiles)
			{
				bool isSelectedTile = resourcePackTile->GetResourcePackName() == resourcePackName;
				resourcePackTile->SetChecked(isSelectedTile);
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

		m_EventHandles.push_back(m_DefaultResourcePack_Tile.OnCheckedChanged.Subscribe(
			[this](const ResourcePackTile& tile) { Handle_ResourcePackTileCheckedChanged(tile); }));
	}

	void ResourcePacksPanel::Handle_ResourcePackTileCheckedChanged(const ResourcePackTile& tile)
	{
		if (tile.IsChecked())
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
					if (resourcePackTile->IsChecked())
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
		std::filesystem::path resourcePacksFolderPath = GetResourcePacksDirectory();
		ShellExecuteA(
			nullptr, "open", "explorer.exe", resourcePacksFolderPath.string().c_str(), nullptr, SW_SHOWNORMAL);
	}

	void ResourcePacksPanel::Handle_Done_Click(const Button& sender)
	{
		m_TimerScanResourcePacksFolder.Stop();
		RequestResourcePackChange.Trigger(GetCurrentlySelectedResourcePack());
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
