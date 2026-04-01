#pragma once

#include <shared/world/world_save/WorldInfos.hpp>

#include <renderer/gui/GuiElement.hpp>
#include <renderer/gui/controls/label/Label.hpp>
#include <renderer/gui/controls/sprite/Sprite.hpp>

#include <onion/DateTime.hpp>
#include <onion/Event.hpp>

#include <glm/glm.hpp>
#include <string>

namespace onion::voxel
{
	class WorldTile : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		WorldTile(const std::string& name, const WorldInfos& worldInfos, Texture texture = Texture());
		~WorldTile();

		// ----- Public API -----
	  public:
		void Render() override;
		void Initialize() override;
		void Delete() override;
		void ReloadTextures() override;

		std::string FormatDescription() const;
		std::string FormatDetails() const;

		// ----- Public Events -----
	  public:
		Event<const WorldTile&> EvtTileSelected;
		Event<const WorldTile&> EvtTileDoubleClicked;

		// ----- Getters / Setters -----
	  public:
		void SetSize(const glm::vec2& size);
		glm::vec2 GetSize() const;

		void SetPosition(const glm::vec2& pos);
		glm::vec2 GetPosition() const;

		void SetWorldInfos(const WorldInfos& worldInfos);
		const WorldInfos GetWorldInfos() const;

		void SetSelected(bool selected);
		bool IsSelected() const;

		void SetThumbnailTexture(Texture& texture);

		// ----- Properties -----
	  private:
		glm::vec2 m_Position{0, 0};
		glm::vec2 m_Size{1, 1};

		bool m_IsSelected = false;

		WorldInfos m_WorldInfos;

		bool m_HasBeenInitialized = false;

		bool m_WasMouseHovering = false;
		bool m_WasMouseDown = false;
		float m_LastClickTime = 0.f;

		static inline std::filesystem::path s_SpriteJoinPathFromRessourcePack =
			GuiElement::s_BasePathGuiAssets / "sprites" / "world_list" / "join.png";

		static inline std::filesystem::path s_SpriteJoinHighlightedPathFromRessourcePack =
			GuiElement::s_BasePathGuiAssets / "sprites" / "world_list" / "join_highlighted.png";

		// ----- Private Helpers -----
	  private:
		bool IsMouseHovering() const;

		// ----- Events Subscription and Handlers -----
	  private:
		std::vector<EventHandle> m_EventHandles;
		void SubscribeToControlEvents();

		// ----- Components -----
	  private:
		Label m_LabelTitle;
		Label m_LabelDescription;
		Label m_LabelDetails;
		Sprite m_SpriteThumbnail;
		Sprite m_SpriteJoin;
		Sprite m_SpriteJoinHighlighted;
	};
}; // namespace onion::voxel
