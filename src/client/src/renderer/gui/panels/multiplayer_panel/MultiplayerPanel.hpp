#pragma once

#include <shared/utils/Utils.hpp>
#include <shared/world/world_save/WorldSave.hpp>

#include <network_client/NetworkClient.hpp>

#include <renderer/gui/GuiElement.hpp>
#include <renderer/gui/controls/button/Button.hpp>
#include <renderer/gui/controls/label/Label.hpp>
#include <renderer/gui/controls/scroller/Scroller.hpp>
#include <renderer/gui/controls/text_field/TextField.hpp>

#include "ServerInfos.hpp"
#include "ServerTile.hpp"

namespace onion::voxel
{
	class MultiplayerPanel : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		MultiplayerPanel(const std::string& name);
		~MultiplayerPanel() override;

		// ----- Public API -----
	  public:
		void RefreshServerTilesAsync();

		void Render() override;
		void Initialize() override;
		void Delete() override;
		void ReloadTextures() override;

		// ----- Public Events -----
	  public:
		Event<const GuiElement*> EvtRequestBackNavigation;
		Event<const ServerInfos&> EvtConnectToServer;

		// ---- Private Enums ----
	  private:
		enum class eRenderModule
		{
			ServerTiles,
			DirectConnect,
			DeleteConfirmation,
			AddEditServer
		};

		// ----- Properties -----
	  private:
		eRenderModule m_CurrentRenderModule = eRenderModule::ServerTiles;

		NetworkClient m_NetworkClient;
		void SubscribeToNetworkClientEvents();
		std::jthread m_RefreshServerTilesThread;
		void RefreshServerTilesAsync(std::stop_token stopToken);
		std::atomic_bool m_AckMotdReceived = false;
		std::chrono::steady_clock::time_point m_MotdRequestTime;

		static const inline std::filesystem::path s_ServersListFilePath =
			Utils::GetExecutableDirectory() / "server_list.bin";

		// ----- Controls World Tiles -----
	  private:
		Label m_LabelTitle;
		Scroller m_Scroller;

		Button m_Button_JoinServer;
		Button m_Button_DirectConnect;
		Button m_Button_AddServer;
		Button m_Button_EditServer;
		Button m_Button_DeleteServer;
		Button m_Button_RefreshServerTiles;
		Button m_Button_Back;

		std::vector<std::unique_ptr<ServerTile>> m_ServerTiles;

		// ---- Controls Delete Confirmation -----
	  private:
		Label m_LabelDeleteWarning;
		Label m_LabelDeleteDetails;

		Button m_ButtonDeleteConfirm;
		Button m_ButtonDeleteCancel;

		// ---- Controls Add / Edit New Server -----
	  private:
		enum class eAddEditMode
		{
			Add,
			Edit
		};

		eAddEditMode m_AddEditMode = eAddEditMode::Add;

		Label m_LabelAddEditTitle;
		Label m_LabelAddEditName;
		TextField m_TextFieldAddEditName;
		Label m_LabelAddEditAddress;
		TextField m_TextFieldAddEditAddress;

		Button m_ButtonAddEditDone;
		Button m_ButtonAddEditCancel;

		// ---- Controls Direct Connect -----
	  private:
		Label m_LabelDirectConnectTitle;
		Label m_LabelDirectConnectAddress;
		TextField m_TextFieldDirectConnectAddress;
		Button m_ButtonDirectConnectJoin;
		Button m_ButtonDirectConnectCancel;

		// ----- Internal Methods -----
	  private:
		void InitializeServerTiles();

		void RenderServerTiles();
		void RenderDeleteConfirmation();
		void RenderAddEditServer();
		void RenderDirectConnect();

		bool IsAnyServerTileSelected() const;

		void ClearServerTiles();
		void SaveServerTiles();

		void UpdateServerTileInfos(ServerTile& serverTile, std::stop_token& stopToken);

		// ----- Internal Event Subscription and Handlers -----
	  private:
		void SubscribeToControlEvents();

		std::vector<EventHandle> m_EventHandles;

		void Handle_ServerTileSelected(const ServerTile& serverTile);
		void Handle_ServerTileDoubleClicked(const ServerTile& serverTile);

		void Handle_ButtonJoinServer_Clicked(const Button& button);
		void Handle_ButtonDirectConnect_Clicked(const Button& button);
		void Handle_ButtonAddServer_Clicked(const Button& button);
		void Handle_ButtonEditServer_Clicked(const Button& button);
		void Handle_ButtonDeleteServer_Clicked(const Button& button);
		void Handle_ButtonRefreshServerTiles_Clicked(const Button& button);
		void Handle_ButtonBack_Clicked(const Button& button);

		void Handle_NetworkClient_Connected(const ServerInfoMsg& serverInfoMsg);
		void Handle_NetworkClient_Disconnected(const bool& val);
		void Handle_NetworkClient_MessageReceived(const NetworkMessage& message);
		void Handle_NetworkClient_ServerMOTDReceived(const ServerMotdMsg& motdMsg);

		void Handle_AddEditDone_Clicked(const Button& button);
		void Handle_AddEditDone_AddMode(const Button& button);
		void Handle_AddEditDone_EditMode(const Button& button);
		void Handle_AddEditCancel_Clicked(const Button& button);

		void Handle_DeleteConfirm_Clicked(const Button& button);
		void Handle_DeleteCancel_Clicked(const Button& button);

		void Handle_DirectConnectJoin_Clicked(const Button& button);
		void Handle_DirectConnectCancel_Clicked(const Button& button);
	};
} // namespace onion::voxel
