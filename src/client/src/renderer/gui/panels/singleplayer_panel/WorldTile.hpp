#pragma once

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
		WorldTile(const std::string& name, Texture texture = Texture());
		~WorldTile();

		// ----- Public API -----
	  public:
		void Render() override;
		void Initialize() override;
		void Delete() override;
		void ReloadTextures() override;

		// ----- Public Events -----
	  public:
		Event<const WorldTile&> EvtTileSelected;

		// ----- Getters / Setters -----
	  public:
		void SetSize(const glm::vec2& size);
		glm::vec2 GetSize() const;

		void SetPosition(const glm::vec2& pos);
		glm::vec2 GetPosition() const;

		void SetSelected(bool selected);
		bool IsSelected() const;

		void SetWorldName(const std::string& name);
		std::string GetWorldName() const;

		void SetWorldDescription(const std::string& description);
		std::string GetWorldDescription() const;

		void SetWorldLastPlayed(const DateTime& lastPlayed);
		DateTime GetWorldLastPlayed() const;

		void SetThumbnailTexture(Texture& texture);

		// ----- Properties -----
	  private:
		glm::vec2 m_Position{0, 0};
		glm::vec2 m_Size{1, 1};

		bool m_IsSelected = false;

		std::string m_WorldName;
		std::string m_WorldDescription;
		DateTime m_WorldLastPlayed;

		bool m_HasBeenInitialized = false;

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
		Label m_LabelLastPlayed;
		Sprite m_SpriteThumbnail;
	};
}; // namespace onion::voxel
