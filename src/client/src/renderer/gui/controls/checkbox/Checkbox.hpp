#pragma once

#include <renderer/gui/GuiElement.hpp>
#include <renderer/gui/controls/sprite/Sprite.hpp>

#include <onion/Event.hpp>

#include <glm/glm.hpp>
#include <string>

namespace onion::voxel
{
	class Checkbox : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		Checkbox(const std::string& name);
		~Checkbox();

		// ----- Public API -----
	  public:
		void Render() override;
		void Initialize() override;
		void Delete() override;
		void ReloadTextures() override;

		// ----- Public Events -----
	  public:
		Event<const Checkbox&> OnCheckedChanged;

		// ----- Getters / Setters -----
	  public:
		/// @brief Sets the size of the checkbox. (In Pixels)
		void SetSize(const glm::vec2& size);
		glm::vec2 GetSize() const;

		/// @brief Sets the center position of the checkbox. (In Pixels)
		void SetPosition(const glm::vec2& pos);
		glm::vec2 GetPosition() const;

		void SetChecked(bool checked);
		bool IsChecked() const;

		void SetVisibility(const Visibility& visibility) override;

		// ----- Properties -----
	  public:
		glm::vec2 m_Position{0, 0};
		glm::vec2 m_Size{1, 1};
		bool m_Checked = false;

		bool m_WasMouseDown = false;

		// ----- Internal Event Subscription and Handlers -----
	  private:
		void SubscribeToSpriteEvents();

		std::vector<EventHandle> m_EventHandles;

		void Handle_Click(const Sprite& sprite);
		void Handle_HoverEnter(const Sprite& sprite);
		void Handle_HoverLeave(const Sprite& sprite);

		// ----- Internal Helpers -----
	  private:
		bool IsHovered() const;

		// ----- Sprites -----
	  private:
		static inline const std::filesystem::path s_SpritePathFromGui =
			GuiElement::s_BasePathGuiAssets / "sprites" / "widget" / "checkbox.png";

		static inline const std::filesystem::path s_SpritePathFromGui_Highlighted =
			GuiElement::s_BasePathGuiAssets / "sprites" / "widget" / "checkbox_highlighted.png";

		static inline const std::filesystem::path s_SpritePathFromGui_Selected =
			GuiElement::s_BasePathGuiAssets / "sprites" / "widget" / "checkbox_selected.png";

		static inline const std::filesystem::path s_SpritePathFromGui_SelectedHighlighted =
			GuiElement::s_BasePathGuiAssets / "sprites" / "widget" / "checkbox_selected_highlighted.png";

		Sprite m_Checkbox_Sprite;
		Sprite m_CheckboxHighlighted_Sprite;
		Sprite m_CheckboxSelected_Sprite;
		Sprite m_CheckboxSelectedHighlighted_Sprite;
	};
}; // namespace onion::voxel
