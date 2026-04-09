#include "MultiplayerPanel.hpp"

#include <renderer/gui/LayoutHelper.hpp>

namespace onion::voxel
{
	MultiplayerPanel::MultiplayerPanel(const std::string& name)
		: GuiElement(name), m_LabelTitle("Title"), m_Scroller("Scroller"), m_Button_JoinServer("Join Server"),
		  m_Button_DirectConnect("Direct Connect"), m_Button_AddServer("Add Server"),
		  m_Button_EditServer("Edit Server"), m_Button_DeleteServer("Delete Server"),
		  m_Button_RefreshServerTiles("Refresh Server Tiles"), m_Button_Back("Back")
	{
		SubscribeToControlEvents();

		// ---- Initialize Controls World Tiles ----
		{
			m_LabelTitle.SetText("Play Multiplayer");
			m_LabelTitle.SetTextAlignment(Font::eTextAlignment::Center);
			m_Button_JoinServer.SetText("Join Server");
			m_Button_JoinServer.SetEnabled(false);
			m_Button_DirectConnect.SetText("Direct Connection");
			m_Button_AddServer.SetText("Add Server");
			m_Button_EditServer.SetText("Edit");
			m_Button_EditServer.SetEnabled(false);
			m_Button_DeleteServer.SetText("Delete");
			m_Button_DeleteServer.SetEnabled(false);
			m_Button_RefreshServerTiles.SetText("Refresh");
			m_Button_Back.SetText("Back");
		}
	}

	MultiplayerPanel::~MultiplayerPanel()
	{
		m_EventHandles.clear();
	}

	void MultiplayerPanel::RefreshServerTiles() {}

	void MultiplayerPanel::Render()
	{
		switch (m_CurrentRenderModule)
		{
			case eRenderModule::ServerTiles:
				RenderServerTiles();
				break;
			case eRenderModule::DirectConnect:
				RenderDirectConnect();
				break;
			case eRenderModule::DeleteConfirmation:
				RenderDeleteConfirmation();
				break;
			case eRenderModule::AddServer:
				RenderAddServer();
				break;
			case eRenderModule::EditServer:
				RenderEditServer();
				break;
			default:
				break;
		}
	}

	void MultiplayerPanel::Initialize()
	{
		m_LabelTitle.Initialize();
		m_Scroller.Initialize();
		m_Button_JoinServer.Initialize();
		m_Button_DirectConnect.Initialize();
		m_Button_AddServer.Initialize();
		m_Button_EditServer.Initialize();
		m_Button_DeleteServer.Initialize();
		m_Button_RefreshServerTiles.Initialize();
		m_Button_Back.Initialize();

		// DEBUG / DEMO PURPOSE: Add some dummy server tiles
		for (int i = 0; i < 10; i++)
		{
			ServerInfos serverInfos;
			serverInfos.Name = "Server " + std::to_string(i + 1);
			serverInfos.Description = "This is a description for server " + std::to_string(i + 1);
			serverInfos.PlayerCount = i * 2;
			serverInfos.MaxPlayerCount = 20;
			serverInfos.Ping = 10 + i * 100;

			if (i == 1)
			{
				serverInfos.Ping = -1;
			}

			serverInfos.Address = "127.0.0." + std::to_string(i + 1);

			m_ServerTiles.push_back(std::make_unique<ServerTile>("ServerTile" + std::to_string(i), serverInfos));

			// Subscribe to server tile events
			m_EventHandles.push_back(m_ServerTiles.back()->EvtTileSelected.Subscribe(
				[this](const ServerTile& serverTile) { Handle_ServerTileSelected(serverTile); }));

			m_EventHandles.push_back(m_ServerTiles.back()->EvtTileDoubleClicked.Subscribe(
				[this](const ServerTile& serverTile) { Handle_ServerTileDoubleClicked(serverTile); }));
		}

		for (const auto& serverTile : m_ServerTiles)
			serverTile->Initialize();

		SetInitState(true);
	}

	void MultiplayerPanel::Delete()
	{
		m_LabelTitle.Delete();
		m_Scroller.Delete();
		m_Button_JoinServer.Delete();
		m_Button_DirectConnect.Delete();
		m_Button_AddServer.Delete();
		m_Button_EditServer.Delete();
		m_Button_DeleteServer.Delete();
		m_Button_RefreshServerTiles.Delete();
		m_Button_Back.Delete();

		for (const auto& serverTile : m_ServerTiles)
			serverTile->Delete();

		SetDeletedState(true);
	}

	void MultiplayerPanel::ReloadTextures()
	{
		m_LabelTitle.ReloadTextures();
		m_Scroller.ReloadTextures();
		m_Button_JoinServer.ReloadTextures();
		m_Button_DirectConnect.ReloadTextures();
		m_Button_AddServer.ReloadTextures();
		m_Button_EditServer.ReloadTextures();
		m_Button_DeleteServer.ReloadTextures();
		m_Button_RefreshServerTiles.ReloadTextures();
		m_Button_Back.ReloadTextures();

		for (const auto& serverTile : m_ServerTiles)
			serverTile->ReloadTextures();
	}

	void MultiplayerPanel::RenderServerTiles()
	{
		if (s_IsBackPressed)
		{
			Handle_ButtonBack_Clicked(m_Button_Back);
			return;
		}

		// ---- Layout Constants ----
		int centerX = static_cast<int>(std::round(s_ScreenWidth / 2.f));

		// ---- Render Title ----
		float titleYOffsetRatio = (87.f - 23.f) / 1009.f;
		int titleY = static_cast<int>(std::round(s_ScreenHeight * titleYOffsetRatio));
		m_LabelTitle.SetPosition({centerX, titleY});
		m_LabelTitle.SetTextHeight(s_TextHeight);
		m_LabelTitle.Render();

		// ---- Render Scroller ----
		float scrollerTopYOffsetRatio = (153.f - 23.f) / 1009.f;
		int scrollerTopY = static_cast<int>(std::round(s_ScreenHeight * scrollerTopYOffsetRatio));
		float scrollerBottomYOffsetRatio = (794.f - 23.f) / 1009.f;
		int scrollerBottomY = static_cast<int>(std::round(s_ScreenHeight * scrollerBottomYOffsetRatio));
		glm::ivec2 scrollerTopLeftCorner{0, scrollerTopY};
		glm::ivec2 scrollerBottomRightCorner{s_ScreenWidth, scrollerBottomY};
		m_Scroller.SetTopLeftCorner(scrollerTopLeftCorner);
		m_Scroller.SetBottomRightCorner(scrollerBottomRightCorner);
		m_Scroller.Render();
		int scrollerYOffset = m_Scroller.GetContentYOffset();

		// ---- Start Scroller Cissoring ----
		m_Scroller.StartCissoring();

		// ---- Render Server Tiles ----
		float tileWidthRatio = 1211.f / 1920.f;
		int tileWidth = static_cast<int>(std::round(s_ScreenWidth * tileWidthRatio));
		float tileHeightRatio = 144.f / 1009.f;
		int tileHeight = static_cast<int>(std::round(s_ScreenHeight * tileHeightRatio));
		float verticalSpacingRatio = 8.f / 1009.f;
		int verticalSpacing = static_cast<int>(std::round(s_ScreenHeight * verticalSpacingRatio));
		glm::ivec2 tilePosition{centerX, scrollerTopY + tileHeight / 2 + verticalSpacing / 2};

		// ---- Update Scroller Height ----
		int tilesCount = static_cast<int>(m_ServerTiles.size());
		int totalTilesHeight = tilesCount * tileHeight + tilesCount * verticalSpacing;
		m_Scroller.SetScrollAreaHeight(totalTilesHeight);

		for (int i = 0; i < tilesCount; i++)
		{
			auto& serverTile = m_ServerTiles[i];
			serverTile->SetSize({tileWidth, tileHeight});
			serverTile->SetPosition(tilePosition - glm::ivec2(0, scrollerYOffset));
			Visibility tileVisibility =
				m_Scroller.GetControlVisibleArea(serverTile->GetPosition(), serverTile->GetSize());
			serverTile->SetVisibility(tileVisibility);
			serverTile->Render();

			tilePosition.y += tileHeight + verticalSpacing;
		}

		// ---- Stop Scroller Cissoring ----
		m_Scroller.StopCissoring();

		// ---- Create Layout for first 3 buttons ----
		float tableWidthRatio = 1232.f / 1920.f;
		int tableWidth = static_cast<int>(std::round(s_ScreenWidth * tableWidthRatio));
		float horizontalSpacingRatio = 16.f / 1920.f;
		int horizontalSpacing = static_cast<int>(std::round(s_ScreenWidth * horizontalSpacingRatio));
		float tableTopLeftYOffsetRatio = (827.f - 23.f) / 1009.f;
		int tableTopLeftY = static_cast<int>(std::round(s_ScreenHeight * tableTopLeftYOffsetRatio));
		int tableTopLeftX = centerX - tableWidth / 2;
		glm::ivec2 tableTopLeftCorner{tableTopLeftX, tableTopLeftY};
		TableLayout layerTopButtons =
			LayoutHelper::CreateTableLayout(1, 3, glm::ivec2{tableWidth, s_ControlHeight}, horizontalSpacing, 0);
		glm::ivec2 cellSize = layerTopButtons.GetCellSize();

		// ---- Render Join Server Button ----
		glm::ivec2 buttonPos = tableTopLeftCorner + layerTopButtons.GetElementPosition(0, 0);
		m_Button_JoinServer.SetPosition(buttonPos);
		m_Button_JoinServer.SetSize(cellSize);
		m_Button_JoinServer.Render();

		// ---- Render Direct Connect Button ----
		buttonPos = tableTopLeftCorner + layerTopButtons.GetElementPosition(0, 1);
		m_Button_DirectConnect.SetPosition(buttonPos);
		m_Button_DirectConnect.SetSize(cellSize);
		m_Button_DirectConnect.Render();

		// ---- Render Add Server Button ----
		buttonPos = tableTopLeftCorner + layerTopButtons.GetElementPosition(0, 2);
		m_Button_AddServer.SetPosition(buttonPos);
		m_Button_AddServer.SetSize(cellSize);
		m_Button_AddServer.Render();

		// -- Create Layout for last 4 buttons ----
		float ySpacingRatio = 16.f / 1009.f;
		int ySpacing = static_cast<int>(std::round(s_ScreenHeight * ySpacingRatio));
		glm::ivec2 table2TopLeftCorner{tableTopLeftX, tableTopLeftY + cellSize.y + ySpacing};
		TableLayout layerBottomButtons =
			LayoutHelper::CreateTableLayout(1, 4, glm::ivec2{tableWidth, s_ControlHeight}, horizontalSpacing, 0);
		cellSize = layerBottomButtons.GetCellSize();

		// ---- Render Edit Server Button ----
		buttonPos = table2TopLeftCorner + layerBottomButtons.GetElementPosition(0, 0);
		m_Button_EditServer.SetPosition(buttonPos);
		m_Button_EditServer.SetSize(cellSize);
		m_Button_EditServer.Render();

		// ---- Render Delete Server Button ----
		buttonPos = table2TopLeftCorner + layerBottomButtons.GetElementPosition(0, 1);
		m_Button_DeleteServer.SetPosition(buttonPos);
		m_Button_DeleteServer.SetSize(cellSize);
		m_Button_DeleteServer.Render();

		// ---- Render Refresh Server Tiles Button ----
		buttonPos = table2TopLeftCorner + layerBottomButtons.GetElementPosition(0, 2);
		m_Button_RefreshServerTiles.SetPosition(buttonPos);
		m_Button_RefreshServerTiles.SetSize(cellSize);
		m_Button_RefreshServerTiles.Render();

		// ---- Render Back Button ----
		buttonPos = table2TopLeftCorner + layerBottomButtons.GetElementPosition(0, 3);
		m_Button_Back.SetPosition(buttonPos);
		m_Button_Back.SetSize(cellSize);
		m_Button_Back.Render();
	}

	void MultiplayerPanel::RenderDeleteConfirmation() {}

	void MultiplayerPanel::RenderEditServer() {}

	void MultiplayerPanel::RenderAddServer() {}

	void MultiplayerPanel::RenderDirectConnect() {}

	void MultiplayerPanel::ClearServerTiles()
	{
		for (const auto& serverTile : m_ServerTiles)
			serverTile->Delete();

		m_ServerTiles.clear();
	}

	void MultiplayerPanel::SubscribeToControlEvents()
	{
		m_EventHandles.push_back(
			m_Button_Back.OnClick.Subscribe([this](const Button& button) { EvtRequestBackNavigation.Trigger(this); }));

		m_EventHandles.push_back(m_Button_JoinServer.OnClick.Subscribe([this](const Button& button)
																	   { Handle_ButtonJoinServer_Clicked(button); }));

		m_EventHandles.push_back(m_Button_DirectConnect.OnClick.Subscribe(
			[this](const Button& button) { Handle_ButtonDirectConnect_Clicked(button); }));

		m_EventHandles.push_back(m_Button_AddServer.OnClick.Subscribe([this](const Button& button)
																	  { Handle_ButtonAddServer_Clicked(button); }));

		m_EventHandles.push_back(m_Button_EditServer.OnClick.Subscribe([this](const Button& button)
																	   { Handle_ButtonEditServer_Clicked(button); }));

		m_EventHandles.push_back(m_Button_DeleteServer.OnClick.Subscribe(
			[this](const Button& button) { Handle_ButtonDeleteServer_Clicked(button); }));

		m_EventHandles.push_back(m_Button_RefreshServerTiles.OnClick.Subscribe(
			[this](const Button& button) { Handle_ButtonRefreshServerTiles_Clicked(button); }));
	}

	void MultiplayerPanel::Handle_ServerTileSelected(const ServerTile& serverTile)
	{
		// Deselect all other tiles
		for (const auto& tile : m_ServerTiles)
		{
			if (tile.get() != &serverTile)
				tile->SetSelected(false);
		}
	}

	void MultiplayerPanel::Handle_ServerTileDoubleClicked(const ServerTile& serverTile)
	{
		(void) serverTile;
		Handle_ButtonJoinServer_Clicked(m_Button_JoinServer);
	}

	void MultiplayerPanel::Handle_ButtonJoinServer_Clicked(const Button& button)
	{
		throw std::logic_error("Not implemented");
	}

	void MultiplayerPanel::Handle_ButtonDirectConnect_Clicked(const Button& button)
	{
		throw std::logic_error("Not implemented");
	}

	void MultiplayerPanel::Handle_ButtonAddServer_Clicked(const Button& button)
	{
		throw std::logic_error("Not implemented");
	}

	void MultiplayerPanel::Handle_ButtonEditServer_Clicked(const Button& button)
	{
		throw std::logic_error("Not implemented");
	}

	void MultiplayerPanel::Handle_ButtonDeleteServer_Clicked(const Button& button)
	{
		throw std::logic_error("Not implemented");
	}

	void MultiplayerPanel::Handle_ButtonRefreshServerTiles_Clicked(const Button& button)
	{
		throw std::logic_error("Not implemented");
	}

	void MultiplayerPanel::Handle_ButtonBack_Clicked(const Button& button)
	{
		(void) button;
		EvtRequestBackNavigation.Trigger(this);
	}

} // namespace onion::voxel
