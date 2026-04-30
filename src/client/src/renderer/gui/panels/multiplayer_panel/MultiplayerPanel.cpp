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
		  m_ButtonAddEditDone("AddEditDone"), m_ButtonAddEditCancel("AddEditCancel"),
		  m_LabelDeleteWarning("Delete Warning"), m_LabelDeleteDetails("Delete Details"),
		  m_ButtonDeleteConfirm("Delete Confirm"), m_ButtonDeleteCancel("Delete Cancel"),
		  m_LabelDirectConnectTitle("DirectConnectTitle"), m_LabelDirectConnectAddress("DirectConnectAddress"),
		  m_TextFieldDirectConnectAddress("DirectConnectAddress"), m_ButtonDirectConnectJoin("DirectConnectJoin"),
		  m_ButtonDirectConnectCancel("DirectConnectCancel")
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

		// ---- Initialize Controls Delete Confirmation -----
		{
			m_LabelDeleteWarning.SetTextAlignment(Font::eTextAlignment::Center);
			m_LabelDeleteDetails.SetTextAlignment(Font::eTextAlignment::Center);

			m_ButtonDeleteConfirm.SetText("Delete");
			m_ButtonDeleteCancel.SetText("Cancel");
		}

		// ---- Initialize Controls Direct Connect -----
		{
			m_LabelDirectConnectTitle.SetText("Direct Connection");
			m_LabelDirectConnectTitle.SetTextAlignment(Font::eTextAlignment::Center);
			m_LabelDirectConnectAddress.SetText("Server Address");
			m_LabelDirectConnectAddress.SetTextColor(Font::eColor::Gray);
			m_LabelDirectConnectAddress.SetTextAlignment(Font::eTextAlignment::Left);
			m_TextFieldDirectConnectAddress.SetPlaceholderText("IP:Port");
			m_ButtonDirectConnectJoin.SetText("Join Server");
			m_ButtonDirectConnectCancel.SetText("Cancel");
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

		m_LabelDeleteWarning.Initialize();
		m_LabelDeleteDetails.Initialize();
		m_ButtonDeleteConfirm.Initialize();
		m_ButtonDeleteCancel.Initialize();

		m_LabelDirectConnectTitle.Initialize();
		m_LabelDirectConnectAddress.Initialize();
		m_TextFieldDirectConnectAddress.Initialize();
		m_ButtonDirectConnectJoin.Initialize();
		m_ButtonDirectConnectCancel.Initialize();

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

		m_LabelDeleteWarning.Delete();
		m_LabelDeleteDetails.Delete();
		m_ButtonDeleteConfirm.Delete();
		m_ButtonDeleteCancel.Delete();

		m_LabelDirectConnectTitle.Delete();
		m_LabelDirectConnectAddress.Delete();
		m_TextFieldDirectConnectAddress.Delete();
		m_ButtonDirectConnectJoin.Delete();
		m_ButtonDirectConnectCancel.Delete();

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

		m_LabelDeleteWarning.ReloadTextures();
		m_LabelDeleteDetails.ReloadTextures();
		m_ButtonDeleteConfirm.ReloadTextures();
		m_ButtonDeleteCancel.ReloadTextures();

		m_LabelDirectConnectTitle.ReloadTextures();
		m_LabelDirectConnectAddress.ReloadTextures();
		m_TextFieldDirectConnectAddress.ReloadTextures();
		m_ButtonDirectConnectJoin.ReloadTextures();
		m_ButtonDirectConnectCancel.ReloadTextures();
	}

	void MultiplayerPanel::SubscribeToNetworkClientEvents()
	{
		m_EventHandles.push_back(m_NetworkClient.EvtConnected.Subscribe(
			[this](const ServerInfoMsg& serverInfoMsg) { Handle_NetworkClient_Connected(serverInfoMsg); }));

		m_EventHandles.push_back(m_NetworkClient.EvtDisconnected.Subscribe(
			[this](const bool& val) { Handle_NetworkClient_Disconnected(val); }));

		m_EventHandles.push_back(m_NetworkClient.EvtMessageReceived.Subscribe(
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
		if (IsBackPressed())
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

	void MultiplayerPanel::RenderDeleteConfirmation()
	{
		// ---- Retreve Selected Server Tile ----
		ServerTile* selectedServerTile = nullptr;
		for (const auto& serverTile : m_ServerTiles)
		{
			if (serverTile->IsSelected())
			{
				selectedServerTile = serverTile.get();
				break;
			}
		}

		if (IsBackPressed() || selectedServerTile == nullptr)
		{
			m_CurrentRenderModule = eRenderModule::ServerTiles;
			return;
		}

		ServerInfos infos = selectedServerTile->GetServerInfos();

		int centerX = s_ScreenWidth / 2;
		float textHeight = s_ScreenHeight * (28.f / 1009.f);

		// ---- Render Warning Text ----
		const std::string warningText = "Are you sure you want to remove this server?";
		float warningTextYOffsetRatio = (400.f - 23.f) / 1009.f;
		glm::ivec2 warningTextPos{centerX, static_cast<int>(s_ScreenHeight * warningTextYOffsetRatio)};
		m_LabelDeleteWarning.SetPosition(warningTextPos);
		m_LabelDeleteWarning.SetText(warningText);
		m_LabelDeleteWarning.SetTextHeight(textHeight);
		m_LabelDeleteWarning.Render();

		// ---- Render Details Text ----
		const std::string detailsText = "'" + infos.Name + "' will be lost forever! (A long time!)";
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

	void MultiplayerPanel::RenderAddEditServer()
	{
		if (IsBackPressed())
		{
			m_CurrentRenderModule = eRenderModule::ServerTiles;
			return;
		}

		bool validInputs = !m_TextFieldAddEditName.GetText().empty() && !m_TextFieldAddEditAddress.GetText().empty();

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

		// ----- Render Done Button ----
		float buttonDoneYOffsetRatio = (770.f - 23.f) / 1009.f;
		int buttonDoneY = static_cast<int>(std::round(s_ScreenHeight * buttonDoneYOffsetRatio));
		m_ButtonAddEditDone.SetPosition({centerX, buttonDoneY});
		m_ButtonAddEditDone.SetSize({controlWidth, s_ControlHeight});
		m_ButtonAddEditDone.SetEnabled(validInputs);
		m_ButtonAddEditDone.Render();

		// ----- Render Cancel Button ----
		float buttonCancelYOffsetRatio = (865.f - 23.f) / 1009.f;
		int buttonCancelY = static_cast<int>(std::round(s_ScreenHeight * buttonCancelYOffsetRatio));
		m_ButtonAddEditCancel.SetPosition({centerX, buttonCancelY});
		m_ButtonAddEditCancel.SetSize({controlWidth, s_ControlHeight});
		m_ButtonAddEditCancel.Render();
	}

	void MultiplayerPanel::RenderDirectConnect()
	{
		if (IsBackPressed())
		{
			m_CurrentRenderModule = eRenderModule::ServerTiles;
			return;
		}

		// ---- Layout Constants ----
		int centerX = static_cast<int>(std::round(s_ScreenWidth / 2.f));
		float controlWidthRatio = 800.f / 1920.f;
		int controlWidth = static_cast<int>(std::round(s_ScreenWidth * controlWidthRatio));
		int textX = centerX - controlWidth / 2;
		bool validInputs = !m_TextFieldDirectConnectAddress.GetText().empty();

		// ---- Render Title ----
		float titleYOffsetRatio = (87.f - 23.f) / 1009.f;
		int titleY = static_cast<int>(std::round(s_ScreenHeight * titleYOffsetRatio));
		m_LabelDirectConnectTitle.SetPosition({centerX, titleY});
		m_LabelDirectConnectTitle.SetTextHeight(s_TextHeight);
		m_LabelDirectConnectTitle.Render();

		// ----- Render Name Label and TextField ----
		float nameYOffsetRatio = (435.f - 23.f) / 1009.f;
		int nameY = static_cast<int>(std::round(s_ScreenHeight * nameYOffsetRatio));
		m_LabelDirectConnectAddress.SetPosition({textX, nameY});
		m_LabelDirectConnectAddress.SetTextHeight(s_TextHeight);
		m_LabelDirectConnectAddress.Render();

		float nameFieldYOffsetRatio = (527.f - 23.f) / 1009.f;
		int nameFieldY = static_cast<int>(std::round(s_ScreenHeight * nameFieldYOffsetRatio));
		m_TextFieldDirectConnectAddress.SetPosition({centerX, nameFieldY});
		m_TextFieldDirectConnectAddress.SetSize({controlWidth, s_ControlHeight});
		m_TextFieldDirectConnectAddress.Render();

		// ----- Render Done Button ----
		float buttonDoneYOffsetRatio = (770.f - 23.f) / 1009.f;
		int buttonDoneY = static_cast<int>(std::round(s_ScreenHeight * buttonDoneYOffsetRatio));
		m_ButtonDirectConnectJoin.SetPosition({centerX, buttonDoneY});
		m_ButtonDirectConnectJoin.SetSize({controlWidth, s_ControlHeight});
		m_ButtonDirectConnectJoin.SetEnabled(validInputs);
		m_ButtonDirectConnectJoin.Render();

		// ----- Render Cancel Button ----
		float buttonCancelYOffsetRatio = (865.f - 23.f) / 1009.f;
		int buttonCancelY = static_cast<int>(std::round(s_ScreenHeight * buttonCancelYOffsetRatio));
		m_ButtonDirectConnectCancel.SetPosition({centerX, buttonCancelY});
		m_ButtonDirectConnectCancel.SetSize({controlWidth, s_ControlHeight});
		m_ButtonDirectConnectCancel.Render();
	}

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
		m_EventHandles.push_back(m_Button_Back.EvtClick.Subscribe(
			[this](const Button& button)
			{
				(void) button;
				EvtRequestBackNavigation.Trigger(this);
			}));

		m_EventHandles.push_back(m_Button_JoinServer.EvtClick.Subscribe([this](const Button& button)
																		{ Handle_ButtonJoinServer_Clicked(button); }));

		m_EventHandles.push_back(m_Button_DirectConnect.EvtClick.Subscribe(
			[this](const Button& button) { Handle_ButtonDirectConnect_Clicked(button); }));

		m_EventHandles.push_back(m_Button_AddServer.EvtClick.Subscribe([this](const Button& button)
																	   { Handle_ButtonAddServer_Clicked(button); }));

		m_EventHandles.push_back(m_Button_EditServer.EvtClick.Subscribe([this](const Button& button)
																		{ Handle_ButtonEditServer_Clicked(button); }));

		m_EventHandles.push_back(m_Button_DeleteServer.EvtClick.Subscribe(
			[this](const Button& button) { Handle_ButtonDeleteServer_Clicked(button); }));

		m_EventHandles.push_back(m_Button_RefreshServerTiles.EvtClick.Subscribe(
			[this](const Button& button) { Handle_ButtonRefreshServerTiles_Clicked(button); }));

		m_EventHandles.push_back(m_ButtonAddEditDone.EvtClick.Subscribe([this](const Button& button)
																		{ Handle_AddEditDone_Clicked(button); }));

		m_EventHandles.push_back(m_ButtonAddEditCancel.EvtClick.Subscribe([this](const Button& button)
																		  { Handle_AddEditCancel_Clicked(button); }));

		m_EventHandles.push_back(m_ButtonDeleteConfirm.EvtClick.Subscribe([this](const Button& button)
																		  { Handle_DeleteConfirm_Clicked(button); }));

		m_EventHandles.push_back(m_ButtonDeleteCancel.EvtClick.Subscribe([this](const Button& button)
																		 { Handle_DeleteCancel_Clicked(button); }));

		m_EventHandles.push_back(m_ButtonDirectConnectJoin.EvtClick.Subscribe(
			[this](const Button& button) { Handle_DirectConnectJoin_Clicked(button); }));

		m_EventHandles.push_back(m_ButtonDirectConnectCancel.EvtClick.Subscribe(
			[this](const Button& button) { Handle_DirectConnectCancel_Clicked(button); }));
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
		m_CurrentRenderModule = eRenderModule::DirectConnect;
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

		m_CurrentRenderModule = eRenderModule::DeleteConfirmation;
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

	void MultiplayerPanel::Handle_DeleteConfirm_Clicked(const Button& button)
	{
		(void) button;

		// Retreves and Deletes the selected tile
		for (int i = 0; i < m_ServerTiles.size(); i++)
		{
			if (m_ServerTiles[i]->IsSelected())
			{
				m_ServerTiles[i]->Delete();
				m_ServerTiles.erase(m_ServerTiles.begin() + i);
				break;
			}
		}

		RefreshServerTilesAsync();
	}

	void MultiplayerPanel::Handle_DeleteCancel_Clicked(const Button& button)
	{
		(void) button;
		m_CurrentRenderModule = eRenderModule::ServerTiles;
	}

	void MultiplayerPanel::Handle_DirectConnectJoin_Clicked(const Button& button)
	{
		(void) button;

		// Retreve infos from field
		ServerInfos serverInfos;
		std::string addressPort = m_TextFieldDirectConnectAddress.GetText();

		// Validate address format
		size_t colonPos = addressPort.find(':');
		if (colonPos == std::string::npos)
		{
			std::cerr << "Invalid address format. Expected format: IP:Port" << std::endl;
			return;
		}

		// Split address and port
		serverInfos.Address = addressPort.substr(0, colonPos);
		serverInfos.Port = static_cast<uint16_t>(std::stoi(addressPort.substr(colonPos + 1)));

		// Go back to server tiles for later
		m_CurrentRenderModule = eRenderModule::ServerTiles;

		// Trigger event to connect to the server
		EvtConnectToServer.Trigger(serverInfos);
	}

	void MultiplayerPanel::Handle_DirectConnectCancel_Clicked(const Button& button)
	{
		(void) button;
		m_CurrentRenderModule = eRenderModule::ServerTiles;
	}

} // namespace onion::voxel
