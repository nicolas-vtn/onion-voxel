#pragma once

#include <shared/world/world_save/WorldSave.hpp>

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
		void RefreshServerTiles();

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
			AddServer,
			EditServer
		};

		// ----- Properties -----
	  private:
		eRenderModule m_CurrentRenderModule = eRenderModule::ServerTiles;

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
		// ---- Controls Add / Edit New Server -----
	  private:
		// ---- Controls Direct Connect -----
	  private:
		// ----- Internal Methods -----
	  private:
		void RenderServerTiles();
		void RenderDeleteConfirmation();
		void RenderEditServer();
		void RenderAddServer();
		void RenderDirectConnect();

		void ClearServerTiles();

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
	};
} // namespace onion::voxel
