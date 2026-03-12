#pragma once

#include "../../GuiElement.hpp"
#include "../sprite/Sprite.hpp"

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

		// ----- Properties -----
	  public:
		glm::vec2 m_Position{0, 0};
		glm::vec2 m_Size{1, 1};
		bool m_Checked = false;

		bool m_WasMouseDown = false;

		// ----- Internal Helpers -----
	  private:
		bool IsHovered() const;

		// ----- Sprites -----
	  private:
		std::filesystem::path m_CheckboxSpritePath =
			GetMinecraftTexturesPath() / "gui" / "sprites" / "widget" / "checkbox.png";
		Sprite m_Checkbox_Sprite;

		std::filesystem::path m_CheckboxHighlightedSpritePath =
			GetMinecraftTexturesPath() / "gui" / "sprites" / "widget" / "checkbox_highlighted.png";
		Sprite m_CheckboxHighlighted_Sprite;

		std::filesystem::path m_CheckboxSelectedSpritePath =
			GetMinecraftTexturesPath() / "gui" / "sprites" / "widget" / "checkbox_selected.png";
		Sprite m_CheckboxSelected_Sprite;

		std::filesystem::path m_CheckboxSelectedHighlightedSpritePath =
			GetMinecraftTexturesPath() / "gui" / "sprites" / "widget" / "checkbox_selected_highlighted.png";
		Sprite m_CheckboxSelectedHighlighted_Sprite;
	};
}; // namespace onion::voxel
