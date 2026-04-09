#include "MultiplayerPanel.hpp"

#include <renderer/gui/LayoutHelper.hpp>

namespace onion::voxel
{
	MultiplayerPanel::MultiplayerPanel(const std::string& name)
		: GuiElement(name), m_LabelTitle("Title"), m_Scroller("Scroller"), m_Button_JoinServer("Join Server"),
		  m_Button_DirectConnect("Direct Connect"), m_Button_AddServer("Add Server"),
		  m_Button_EditServer("Edit Server"), m_Button_DeleteServer("Delete Server"),
		  m_Button_RefreshServerTiles("Refresh Server Tiles"), m_Button_Back("Back"),
		  m_LabelAddEditTitle("AddEditTitle"), m_LabelAddEditName("AddEditName"), m_TextFieldAddEditName("AddEditName"),
		  m_LabelAddEditAddress("AddEditAddress"), m_TextFieldAddEditAddress("AddEditAddress"),
		  m_ButtonAddEditDone("AddEditDone"), m_ButtonAddEditCancel("AddEditCancel")
	{
		SubscribeToControlEvents();
		SubscribeToNetworkClientEvents();

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

		// ---- Initialize Controls Add / Edit New Server ----
		{
			m_LabelAddEditTitle.SetText("Add Server");
			m_LabelAddEditTitle.SetTextAlignment(Font::eTextAlignment::Center);
			m_LabelAddEditName.SetText("Server Name");
			m_LabelAddEditName.SetTextColor(Font::eColor::Gray);
			m_LabelAddEditName.SetTextAlignment(Font::eTextAlignment::Left);
			m_LabelAddEditAddress.SetText("Server Address");
			m_LabelAddEditAddress.SetTextColor(Font::eColor::Gray);
			m_LabelAddEditAddress.SetTextAlignment(Font::eTextAlignment::Left);
			m_TextFieldAddEditAddress.SetPlaceholderText("IP:Port");
			m_TextFieldAddEditName.SetPlaceholderText("My Server");
			m_ButtonAddEditDone.SetText("Done");
			m_ButtonAddEditCancel.SetText("Cancel");
		}
	}

	MultiplayerPanel::~MultiplayerPanel()
	{
		m_EventHandles.clear();
	}

	void MultiplayerPanel::RefreshServerTilesAsync()
	{
		if (m_RefreshServerTilesThread.joinable())
		{
			m_RefreshServerTilesThread.request_stop();
			m_RefreshServerTilesThread.join();
		}

		m_RefreshServerTilesThread =
			std::jthread([this](std::stop_token stopToken) { RefreshServerTilesAsync(stopToken); });
	}

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
			case eRenderModule::AddEditServer:
				RenderAddEditServer();
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

		InitializeServerTiles();

		m_LabelAddEditTitle.Initialize();
		m_LabelAddEditName.Initialize();
		m_TextFieldAddEditName.Initialize();
		m_LabelAddEditAddress.Initialize();
		m_TextFieldAddEditAddress.Initialize();
		m_ButtonAddEditDone.Initialize();
		m_ButtonAddEditCancel.Initialize();

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

		m_LabelAddEditTitle.Delete();
		m_LabelAddEditName.Delete();
		m_TextFieldAddEditName.Delete();
		m_LabelAddEditAddress.Delete();
		m_TextFieldAddEditAddress.Delete();
		m_ButtonAddEditDone.Delete();
		m_ButtonAddEditCancel.Delete();

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

		m_LabelAddEditTitle.ReloadTextures();
		m_LabelAddEditName.ReloadTextures();
		m_TextFieldAddEditName.ReloadTextures();
		m_LabelAddEditAddress.ReloadTextures();
		m_TextFieldAddEditAddress.ReloadTextures();
		m_ButtonAddEditDone.ReloadTextures();
		m_ButtonAddEditCancel.ReloadTextures();
	}

	void MultiplayerPanel::SubscribeToNetworkClientEvents()
	{
		m_EventHandles.push_back(m_NetworkClient.Connected.Subscribe(
			[this](const ServerInfoMsg& serverInfoMsg) { Handle_NetworkClient_Connected(serverInfoMsg); }));

		m_EventHandles.push_back(m_NetworkClient.Disconnected.Subscribe([this](const bool& val)
																		{ Handle_NetworkClient_Disconnected(val); }));

		m_EventHandles.push_back(m_NetworkClient.MessageReceived.Subscribe(
			[this](const NetworkMessage& message) { Handle_NetworkClient_MessageReceived(message); }));
	}

	void MultiplayerPanel::RefreshServerTilesAsync(std::stop_token stopToken)
	{
		for (auto& serverTile : m_ServerTiles)
		{
			if (stopToken.stop_requested())
				return;

			UpdateServerTileInfos(*serverTile, stopToken);
		}

		SaveServerTiles();
	}

	void MultiplayerPanel::InitializeServerTiles()
	{
		// Delete existing tiles
		for (const auto& serverTile : m_ServerTiles)
			serverTile->Delete();

		m_ServerTiles.clear();

		// Retreves server list.
		std::vector<ServerInfos> serversList;
		LoadServers(serversList, s_ServersListFilePath.string());

		// Create new tiles
		for (const auto& serverInfos : serversList)
		{
			m_ServerTiles.push_back(std::make_unique<ServerTile>("ServerTile_" + serverInfos.Name, serverInfos));

			// Subscribe to server tile events
			m_EventHandles.push_back(m_ServerTiles.back()->EvtTileSelected.Subscribe(
				[this](const ServerTile& serverTile) { Handle_ServerTileSelected(serverTile); }));
			m_EventHandles.push_back(m_ServerTiles.back()->EvtTileDoubleClicked.Subscribe(
				[this](const ServerTile& serverTile) { Handle_ServerTileDoubleClicked(serverTile); }));
		}

		// Initialize new tiles
		for (const auto& serverTile : m_ServerTiles)
			serverTile->Initialize();
	}

	void MultiplayerPanel::RenderServerTiles()
	{
		if (s_IsBackPressed)
		{
			Handle_ButtonBack_Clicked(m_Button_Back);
			return;
		}

		bool isAnyTileSelected = IsAnyServerTileSelected();

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
		m_Button_JoinServer.SetEnabled(isAnyTileSelected);
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
		m_Button_EditServer.SetEnabled(isAnyTileSelected);
		m_Button_EditServer.Render();

		// ---- Render Delete Server Button ----
		buttonPos = table2TopLeftCorner + layerBottomButtons.GetElementPosition(0, 1);
		m_Button_DeleteServer.SetPosition(buttonPos);
		m_Button_DeleteServer.SetSize(cellSize);
		m_Button_DeleteServer.SetEnabled(isAnyTileSelected);
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

	void MultiplayerPanel::RenderAddEditServer()
	{
		if (s_IsBackPressed)
		{
			m_CurrentRenderModule = eRenderModule::ServerTiles;
			return;
		}

		// ---- Layout Constants ----
		int centerX = static_cast<int>(std::round(s_ScreenWidth / 2.f));
		float controlWidthRatio = 800.f / 1920.f;
		int controlWidth = static_cast<int>(std::round(s_ScreenWidth * controlWidthRatio));
		int textX = centerX - controlWidth / 2;

		// ---- Render Title ----
		float titleYOffsetRatio = (87.f - 23.f) / 1009.f;
		int titleY = static_cast<int>(std::round(s_ScreenHeight * titleYOffsetRatio));
		m_LabelAddEditTitle.SetPosition({centerX, titleY});
		m_LabelAddEditTitle.SetTextHeight(s_TextHeight);
		m_LabelAddEditTitle.Render();

		// ----- Render Name Label and TextField ----
		float nameYOffsetRatio = (253.f - 23.f) / 1009.f;
		int nameY = static_cast<int>(std::round(s_ScreenHeight * nameYOffsetRatio));
		m_LabelAddEditName.SetPosition({textX, nameY});
		m_LabelAddEditName.SetTextHeight(s_TextHeight);
		m_LabelAddEditName.Render();

		float nameFieldYOffsetRatio = (323.f - 23.f) / 1009.f;
		int nameFieldY = static_cast<int>(std::round(s_ScreenHeight * nameFieldYOffsetRatio));
		m_TextFieldAddEditName.SetPosition({centerX, nameFieldY});
		m_TextFieldAddEditName.SetSize({controlWidth, s_ControlHeight});
		m_TextFieldAddEditName.Render();

		// ----- Render Address Label and TextField ----
		float addressYOffsetRatio = (411.f - 23.f) / 1009.f;
		int addressY = static_cast<int>(std::round(s_ScreenHeight * addressYOffsetRatio));
		m_LabelAddEditAddress.SetPosition({textX, addressY});
		m_LabelAddEditAddress.SetTextHeight(s_TextHeight);
		m_LabelAddEditAddress.Render();

		float addressFieldYOffsetRatio = (485.f - 23.f) / 1009.f;
		int addressFieldY = static_cast<int>(std::round(s_ScreenHeight * addressFieldYOffsetRatio));
		m_TextFieldAddEditAddress.SetPosition({centerX, addressFieldY});
		m_TextFieldAddEditAddress.SetSize({controlWidth, s_ControlHeight});
		m_TextFieldAddEditAddress.Render();

		// ----- Render Done and Cancel Buttons ----
		float buttonDoneYOffsetRatio = (770.f - 23.f) / 1009.f;
		int buttonDoneY = static_cast<int>(std::round(s_ScreenHeight * buttonDoneYOffsetRatio));
		m_ButtonAddEditDone.SetPosition({centerX, buttonDoneY});
		m_ButtonAddEditDone.SetSize({controlWidth, s_ControlHeight});
		m_ButtonAddEditDone.Render();

		float buttonCancelYOffsetRatio = (865.f - 23.f) / 1009.f;
		int buttonCancelY = static_cast<int>(std::round(s_ScreenHeight * buttonCancelYOffsetRatio));
		m_ButtonAddEditCancel.SetPosition({centerX, buttonCancelY});
		m_ButtonAddEditCancel.SetSize({controlWidth, s_ControlHeight});
		m_ButtonAddEditCancel.Render();
	}

	void MultiplayerPanel::RenderDirectConnect() {}

	bool MultiplayerPanel::IsAnyServerTileSelected() const
	{
		for (auto& serverTile : m_ServerTiles)
		{
			if (serverTile->IsSelected())
				return true;
		}

		return false;
	}

	void MultiplayerPanel::ClearServerTiles()
	{
		for (const auto& serverTile : m_ServerTiles)
			serverTile->Delete();

		m_ServerTiles.clear();
	}

	void MultiplayerPanel::SaveServerTiles()
	{
		// Save Tiles
		std::vector<ServerInfos> serversInfos;
		for (const auto& serverTile : m_ServerTiles)
			serversInfos.push_back(serverTile->GetServerInfos());

		SaveServers(serversInfos, s_ServersListFilePath.string());
	}

	void MultiplayerPanel::UpdateServerTileInfos(ServerTile& serverTile, std::stop_token& stopToken)
	{
		// Retreve the infos
		ServerInfos serverInfos = serverTile.GetServerInfos();

		// Start Pinging
		serverTile.SetPinging(true);

		m_NetworkClient.Stop();
		m_NetworkClient.SetRemoteHost(serverInfos.Address);
		m_NetworkClient.SetRemotePort(serverInfos.Port);
		m_NetworkClient.Start();

		std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Wait a bit for the connection to establish

		RequestMotdMsg requestMotdMsg;
		m_NetworkClient.Send(requestMotdMsg);

		// Get current time to measure ping
		m_MotdRequestTime = std::chrono::steady_clock::now();

		m_AckMotdReceived = false;
		for (int i = 0; i < 100; i++)
		{
			if (stopToken.stop_requested() || m_AckMotdReceived)
			{
				break;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		if (!m_AckMotdReceived)
		{
			// Set ping to -1
			serverInfos = serverTile.GetServerInfos();
			serverInfos.Ping = -1;
			serverTile.SetServerInfos(serverInfos);
		}

		serverTile.SetPinging(false);

		m_NetworkClient.Stop();
	}

	void MultiplayerPanel::SubscribeToControlEvents()
	{
		m_EventHandles.push_back(m_Button_Back.OnClick.Subscribe(
			[this](const Button& button)
			{
				(void) button;
				EvtRequestBackNavigation.Trigger(this);
			}));

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

		m_EventHandles.push_back(m_ButtonAddEditDone.OnClick.Subscribe([this](const Button& button)
																	   { Handle_AddEditDone_Clicked(button); }));

		m_EventHandles.push_back(m_ButtonAddEditCancel.OnClick.Subscribe([this](const Button& button)
																		 { Handle_AddEditCancel_Clicked(button); }));
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
		(void) button;

		for (const auto& serverTile : m_ServerTiles)
		{
			if (serverTile->IsSelected())
			{
				EvtConnectToServer.Trigger(serverTile->GetServerInfos());
				break;
			}
		}
	}

	void MultiplayerPanel::Handle_ButtonDirectConnect_Clicked(const Button& button)
	{
		(void) button;
		throw std::logic_error("Not implemented");
	}

	void MultiplayerPanel::Handle_ButtonAddServer_Clicked(const Button& button)
	{
		(void) button;

		m_CurrentRenderModule = eRenderModule::AddEditServer;
		m_AddEditMode = eAddEditMode::Add;

		// Set Title
		m_LabelAddEditTitle.SetText("Add Server");

		// Clear Add/Edit Fields
		m_TextFieldAddEditName.SetText("");
		m_TextFieldAddEditAddress.SetText("");
	}

	void MultiplayerPanel::Handle_ButtonEditServer_Clicked(const Button& button)
	{
		(void) button;

		m_CurrentRenderModule = eRenderModule::AddEditServer;
		m_AddEditMode = eAddEditMode::Edit;

		// Retreve the selected tile
		ServerTile* selectedTile = nullptr;
		for (const auto& serverTile : m_ServerTiles)
		{
			if (serverTile->IsSelected())
			{
				selectedTile = serverTile.get();
				break;
			}
		}

		if (!selectedTile)
			throw std::logic_error("No server tile selected for editing");

		// Set Title
		m_LabelAddEditTitle.SetText("Edit Server Info");

		// Fill Add/Edit Fields with selected tile infos
		ServerInfos serverInfos = selectedTile->GetServerInfos();
		m_TextFieldAddEditName.SetText(serverInfos.Name);
		m_TextFieldAddEditAddress.SetText(serverInfos.Address + ":" + std::to_string(serverInfos.Port));
	}

	void MultiplayerPanel::Handle_ButtonDeleteServer_Clicked(const Button& button)
	{
		(void) button;

		for (int i = 0; i < m_ServerTiles.size(); i++)
		{
			if (m_ServerTiles[i]->IsSelected())
			{
				m_ServerTiles[i]->Delete();
				m_ServerTiles.erase(m_ServerTiles.begin() + i);
				break;
			}
		}
	}

	void MultiplayerPanel::Handle_ButtonRefreshServerTiles_Clicked(const Button& button)
	{
		(void) button;
		RefreshServerTilesAsync();
	}

	void MultiplayerPanel::Handle_ButtonBack_Clicked(const Button& button)
	{
		(void) button;
		EvtRequestBackNavigation.Trigger(this);
	}

	void MultiplayerPanel::Handle_NetworkClient_Connected(const ServerInfoMsg& serverInfoMsg)
	{
		(void) serverInfoMsg;
		std::cout << " MultiplayerPanel - Connected to server: " << serverInfoMsg.ServerName << std::endl;
	}

	void MultiplayerPanel::Handle_NetworkClient_Disconnected(const bool& val)
	{
		(void) val;
		std::cout << " MultiplayerPanel - Disconnected from server." << std::endl;
	}

	void MultiplayerPanel::Handle_NetworkClient_MessageReceived(const NetworkMessage& message)
	{
		std::visit(
			[this](const auto& msg)
			{
				using T = std::decay_t<decltype(msg)>;
				if constexpr (std::is_same_v<T, ServerMotdMsg>)
				{
					Handle_NetworkClient_ServerMOTDReceived(msg);
				}
			},
			message);
	}

	void MultiplayerPanel::Handle_NetworkClient_ServerMOTDReceived(const ServerMotdMsg& motdMsg)
	{
		std::string networkHost = m_NetworkClient.GetRemoteHost();
		uint16_t networkPort = m_NetworkClient.GetRemotePort();

		for (auto& serverTile : m_ServerTiles)
		{
			ServerInfos serverInfos = serverTile->GetServerInfos();
			if (networkHost == serverInfos.Address && networkPort == serverInfos.Port)
			{
				serverInfos.Description = motdMsg.ServerMotd;
				serverInfos.PlayerCount = motdMsg.PlayerCount;
				serverInfos.MaxPlayerCount = motdMsg.MaxPlayers;
				serverInfos.PlayerNames = motdMsg.PlayerNames;
				serverInfos.IconPngData = motdMsg.ServerIconPngData;

				// Measure Ping
				auto now = std::chrono::steady_clock::now();
				auto pingDuration = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_MotdRequestTime);
				serverInfos.Ping = static_cast<int>(pingDuration.count());

				std::cout << "MultiplayerPanel - Received MOTD from server: " << serverInfos.Name
						  << " | Ping: " << serverInfos.Ping << "ms" << std::endl;

				serverTile->SetServerInfos(serverInfos);
				m_AckMotdReceived = true;
			}
		}
	}

	void MultiplayerPanel::Handle_AddEditDone_Clicked(const Button& button)
	{
		switch (m_AddEditMode)
		{
			case eAddEditMode::Add:
				Handle_AddEditDone_AddMode(button);
				break;
			case eAddEditMode::Edit:
				Handle_AddEditDone_EditMode(button);
				break;
			default:
				throw std::logic_error("Invalid Add/Edit mode");
		}
	}

	void MultiplayerPanel::Handle_AddEditDone_AddMode(const Button& button)
	{
		(void) button;

		// Retreve infos from fields
		std::string name = m_TextFieldAddEditName.GetText();
		std::string addressPort = m_TextFieldAddEditAddress.GetText();

		size_t colonPos = addressPort.find(':');
		if (colonPos == std::string::npos)
		{
			std::cerr << "Invalid address format. Expected format: IP:Port" << std::endl;
			return;
		}

		// Split address and port
		std::string address = addressPort.substr(0, colonPos);
		uint16_t port = static_cast<uint16_t>(std::stoi(addressPort.substr(colonPos + 1)));

		// Create new ServerInfos
		ServerInfos newServerInfos;
		newServerInfos.Name = name;
		newServerInfos.Address = address;
		newServerInfos.Port = port;

		// Create new ServerTile
		m_ServerTiles.push_back(std::make_unique<ServerTile>("ServerTile_" + name, newServerInfos));

		// Subscribe to server tile events
		m_EventHandles.push_back(m_ServerTiles.back()->EvtTileSelected.Subscribe(
			[this](const ServerTile& serverTile) { Handle_ServerTileSelected(serverTile); }));
		m_EventHandles.push_back(m_ServerTiles.back()->EvtTileDoubleClicked.Subscribe(
			[this](const ServerTile& serverTile) { Handle_ServerTileDoubleClicked(serverTile); }));

		// Initialize new tile
		m_ServerTiles.back()->Initialize();

		// Set current render module to server tiles to go back to the list
		m_CurrentRenderModule = eRenderModule::ServerTiles;

		// Force refresh to retreve the motd and ping of the new server (and saves the new server list to file)
		RefreshServerTilesAsync();
	}

	void MultiplayerPanel::Handle_AddEditDone_EditMode(const Button& button)
	{
		(void) button;

		// Retreve the selected tile
		ServerTile* selectedTile = nullptr;
		for (const auto& serverTile : m_ServerTiles)
		{
			if (serverTile->IsSelected())
			{
				selectedTile = serverTile.get();
				break;
			}
		}

		if (!selectedTile)
			throw std::logic_error("No server tile selected for editing");

		// Retreve infos from fields
		std::string name = m_TextFieldAddEditName.GetText();
		std::string addressPort = m_TextFieldAddEditAddress.GetText();

		size_t colonPos = addressPort.find(':');
		if (colonPos == std::string::npos)
		{
			std::cerr << "Invalid address format. Expected format: IP:Port" << std::endl;
			return;
		}

		// Split address and port
		std::string address = addressPort.substr(0, colonPos);
		uint16_t port = static_cast<uint16_t>(std::stoi(addressPort.substr(colonPos + 1)));

		ServerInfos serverInfos = selectedTile->GetServerInfos();
		serverInfos.Name = name;
		serverInfos.Address = address;
		serverInfos.Port = port;
		selectedTile->SetServerInfos(serverInfos);

		// Set current render module to server tiles to go back to the list
		m_CurrentRenderModule = eRenderModule::ServerTiles;

		// Force refresh to retreve the motd and ping of the edited server
		RefreshServerTilesAsync();
	}

	void MultiplayerPanel::Handle_AddEditCancel_Clicked(const Button& button)
	{
		(void) button;
		m_CurrentRenderModule = eRenderModule::ServerTiles;
	}

} // namespace onion::voxel
