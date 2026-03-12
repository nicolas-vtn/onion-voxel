#pragma once

#include "../../GuiElement.hpp"
#include "../../controls/checkbox/Checkbox.hpp"
#include "../../controls/label/Label.hpp"
#include "../../controls/sprite/Sprite.hpp"

#include <onion/Event.hpp>

#include <glm/glm.hpp>
#include <string>

namespace onion::voxel
{
	class ResourcePackTile : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		ResourcePackTile(const std::string& name, Texture texture);
		~ResourcePackTile();

		// ----- Public API -----
	  public:
		void Render() override;
		void Initialize() override;
		void Delete() override;

		// ----- Public Events -----
	  public:
		Event<const ResourcePackTile&> OnCheckedChanged;

		// ----- Getters / Setters -----
	  public:
		/// @brief Sets the size of the ResourcePackTile. (In Pixels)
		void SetSize(const glm::vec2& size);
		glm::vec2 GetSize() const;

		/// @brief Sets the center position of the ResourcePackTile. (In Pixels)
		void SetPosition(const glm::vec2& pos);
		glm::vec2 GetPosition() const;

		void SetChecked(bool checked);
		bool IsChecked() const;

		void SetResourcePackName(const std::string& name);
		std::string GetResourcePackName() const;

		void SetResourcePackDescription(const std::string& description);
		std::string GetResourcePackDescription() const;

		// ----- Properties -----
	  private:
		glm::vec2 m_Position{0, 0};
		glm::vec2 m_Size{1, 1};

		bool m_HasBeenInitialized = false;

		// ----- Events Subscription and Handlers -----
	  private:
		std::vector<EventHandle> m_EventHandles;
		void SubscribeToControlEvents();

		void Handle_CheckboxCheckedChanged(const Checkbox& checkbox);

		// ----- Components -----
	  private:
		Checkbox m_Checkbox;
		Sprite m_Thumbnail;
		Label m_NameLabel;
		Label m_DescriptionLabel;
	};
}; // namespace onion::voxel
