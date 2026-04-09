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
		ServerTile(const std::string& name, const ServerInfos& serverInfos);
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

		static inline std::filesystem::path s_BaseSpritesPath =
			GuiElement::s_BasePathGuiAssets / "sprites" / "server_list";

		static inline std::vector<std::filesystem::path> s_PingSpritesPaths = {s_BaseSpritesPath / "ping_1.png",
																			   s_BaseSpritesPath / "ping_2.png",
																			   s_BaseSpritesPath / "ping_3.png",
																			   s_BaseSpritesPath / "ping_4.png",
																			   s_BaseSpritesPath / "ping_5.png"};

		static inline std::vector<std::filesystem::path> s_PingingSpritesPaths = {s_BaseSpritesPath / "pinging_1.png",
																				  s_BaseSpritesPath / "pinging_2.png",
																				  s_BaseSpritesPath / "pinging_3.png",
																				  s_BaseSpritesPath / "pinging_4.png",
																				  s_BaseSpritesPath / "pinging_5.png"};

		static inline std::filesystem::path s_UnreachableSpritePath = s_BaseSpritesPath / "unreachable.png";

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

		Sprite m_ThumbnailSprite;

		std::array<Sprite, 5> m_PingSprites;
		std::array<Sprite, 5> m_PingingSprites;
		Sprite m_UnreachableSprite;
	};
}; // namespace onion::voxel
