#pragma once

#include <glm/glm.hpp>

#include <string>

#include <renderer/gui/GuiElement.hpp>
#include <renderer/gui/controls/label/Label.hpp>
#include <renderer/gui/nine_slice_sprite/NineSliceSprite.hpp>

namespace onion::voxel
{
	class Tooltip : public GuiElement
	{
		// ----- Constructor -----
	  public:
		Tooltip(const std::string& name);

		// ----- Public API -----
	  public:
		void Initialize() override;
		void Render() override;
		void Delete() override;
		void ReloadTextures() override;

		// ----- Getters / Setters -----
	  public:
		void SetText(const std::string& text);
		std::string GetText() const;

		/// @brief Sets the anchor position of the tooltip in pixels (screen space).
		/// Normally the tooltip grows rightward from this point (left-center anchor).
		/// If the tooltip would overflow the right screen edge, it automatically flips
		/// and grows leftward (right-center anchor). The caller always passes the same
		/// point (e.g. cursor tip) regardless of flip state.
		void SetPosition(const glm::vec2& position);
		glm::vec2 GetPosition() const;

		/// @brief Sets the tooltip to center mode. In this mode the tooltip is always
		/// horizontally centered on the screen and m_Position.y is the vertical center of the box.
		/// The X component of position is ignored.
		void SetPositionCentered(const glm::vec2& position);

		/// @brief Returns the inner top-left pixel of the tooltip (i.e. the content area after padding),
		/// based on the current text, text height, and position. Useful for placing overlays (e.g. a block mesh)
		/// inside the tooltip before calling Render().
		/// @note Call SetText() and SetTextHeight() before calling this.
		glm::ivec2 GetInnerTopLeft();

		/// @brief Sets the text height in pixels. Controls both font size and tooltip height.
		void SetTextHeight(float textHeight);
		float GetTextHeight() const;

		/// @brief Sets the base Z offset for the background layer.
		/// Frame renders at ZOffset + DeltaZ, label at ZOffset + 2 * DeltaZ.
		void SetZOffset(float zOffset);
		float GetZOffset() const;

		/// @brief Sets the Z spacing between layers (background → frame → label).
		void SetDeltaZ(float deltaZ);
		float GetDeltaZ() const;

		// ----- Properties -----
	  private:
		glm::vec2 m_Position{0.f, 0.f};
		bool m_Centered{false};
		float m_TextHeight{16.f};
		float m_ZOffset{0.f};
		float m_DeltaZ{0.01f};

		// ----- Children -----
	  private:
		NineSliceSprite m_Background;
		NineSliceSprite m_Frame;
		Label m_Label;

		// ----- Static Helpers -----
	  private:
		static inline const std::filesystem::path s_SpritePathBackground =
			GuiElement::s_BasePathGuiAssets / "sprites" / "tooltip" / "background.png";

		static inline const std::filesystem::path s_SpritePathFrame =
			GuiElement::s_BasePathGuiAssets / "sprites" / "tooltip" / "frame.png";
	};

} // namespace onion::voxel
