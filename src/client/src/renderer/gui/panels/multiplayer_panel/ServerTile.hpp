#pragma once

#include <string>

#include <glm/glm.hpp>

#include <onion/DateTime.hpp>
#include <onion/Event.hpp>

#include <renderer/gui/GuiElement.hpp>
#include <renderer/gui/controls/label/Label.hpp>
#include <renderer/gui/controls/sprite/Sprite.hpp>

#include "ServerInfos.hpp"

namespace onion::voxel
{
	class ServerTile : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		ServerTile(const std::string& name, const ServerInfos& serverInfos, Texture texture = Texture());
		~ServerTile();

		// ----- Public API -----
	  public:
		void Render() override;
		void Initialize() override;
		void Delete() override;
		void ReloadTextures() override;

		// ----- Public Events -----
	  public:
		Event<const ServerTile&> EvtTileSelected;
		Event<const ServerTile&> EvtTileDoubleClicked;

		// ----- Getters / Setters -----
	  public:
		void SetSize(const glm::vec2& size);
		glm::vec2 GetSize() const;

		void SetPosition(const glm::vec2& pos);
		glm::vec2 GetPosition() const;

		void SetServerInfos(const ServerInfos& serverInfos);
		const ServerInfos GetServerInfos() const;

		void SetSelected(bool selected);
		bool IsSelected() const;

		void SetThumbnailTexture(Texture& texture);

		// ----- Properties -----
	  private:
		glm::vec2 m_Position{0, 0};
		glm::vec2 m_Size{1, 1};

		bool m_IsSelected = false;

		ServerInfos m_ServerInfos;

		bool m_HasBeenInitialized = false;

		bool m_WasMouseHovering = false;
		bool m_WasMouseDown = false;
		float m_LastClickTime = 0.f;

		//static inline std::filesystem::path s_SpriteJoinPathFromRessourcePack =
		//	GuiElement::s_BasePathGuiAssets / "sprites" / "world_list" / "join.png";

		//static inline std::filesystem::path s_SpriteJoinHighlightedPathFromRessourcePack =
		//	GuiElement::s_BasePathGuiAssets / "sprites" / "world_list" / "join_highlighted.png";

		// ----- Private Helpers -----
	  private:
		bool IsMouseHovering() const;

		// ----- Events Subscription and Handlers -----
	  private:
		std::vector<EventHandle> m_EventHandles;
		void SubscribeToControlEvents();

		// ----- Components -----
	  private:
		Label m_LabelName;
		Label m_LabelDescription;
		Label m_LabelPlayerCount;
		//Label m_LabelDetails;
		//Sprite m_SpriteThumbnail;
		//Sprite m_SpriteJoin;
		//Sprite m_SpriteJoinHighlighted;
	};
}; // namespace onion::voxel
