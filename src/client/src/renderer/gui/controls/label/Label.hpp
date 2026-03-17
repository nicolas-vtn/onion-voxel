#pragma once

#include <glm/glm.hpp>

#include <mutex>
#include <string>

#include <onion/Event.hpp>

#include <renderer/OpenGL.hpp>

#include <renderer/gui/GuiElement.hpp>

namespace onion::voxel
{
	class Label : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		Label(const std::string& name);
		~Label();

		// ----- Public API -----
	  public:
		void Render() override;
		void Initialize() override;
		void Delete() override;
		void ReloadTextures() override;

		// ----- Getters / Setters -----
	  public:
		void SetText(const std::string& text);
		std::string GetText() const;

		/// @brief Sets the position of the label. The position is interpreted as the center of the text for Center alignment, the left-center edge for Left alignment, and the right-center edge for Right alignment.
		/// @param pos The new position of the label in pixels.
		void SetPosition(const glm::vec2& pos);
		glm::vec2 GetPosition() const;

		/// @brief Sets the height of the text in pixels.
		/// @param height The new height of the text in pixels.
		void SetTextHeight(float height);
		float GetTextHeight() const;

		/// @brief Gets the size of the text in pixels. This can be used to calculate the position to render the text based on the desired alignment.
		glm::ivec2 GetTextSize() const;

		/// @brief Sets the alignment of the text relative to the position.
		/// @param alignment The new text alignment (Left, Center, Right).
		void SetTextAlignment(Font::eTextAlignment alignment);
		Font::eTextAlignment GetTextAlignment() const;

		/// @brief Sets the color of the text. The color is specified as a vec3. Red, Green, and Blue components should be in the range [0, 1]. Alpha is implicitly set to 1.
		/// @param color The new color of the text.
		void SetTextColor(const glm::vec3& color);

		/// @brief Sets the color of the text. The color is specified as a vec4. Red, Green, Blue, and Alpha components should be in the range [0, 1]
		/// @param color The new color of the text.
		void SetTextColor(const glm::vec4& color);
		glm::vec4 GetTextColor() const;

		/// @brief Sets the color of the shadow. The color is specified as a vec4. Red, Green, Blue, and Alpha components should be in the range [0, 1]
		/// @param color The new color of the shadow.
		void SetShadowColor(const glm::vec4& color);
		void SetShadowColor(const glm::vec3& color);
		void ResetShadowColor();
		glm::vec4 GetShadowColor() const;

		/// @brief Sets the offset of the text in the Z direction. In range [-1, 1], where 1 is the closest to the camera and -1 is the farthest from the camera.
		/// @param zOffset The new offset of the text in the Z direction.
		void SetZOffset(float zOffset);
		float GetZOffset() const;

		/// @brief Sets the rotation of the text in degrees. In range [0, 360), where 0 is no rotation and the rotation is applied clockwise.
		/// @param rotationDegrees The new rotation of the text in degrees.
		void SetRotationDegrees(float rotationDegrees);
		float GetRotationDegrees() const;

		/// @brief Enables or disables the shadow for the text.
		/// @param enable True to enable the shadow, false to disable it.
		void EnableShadow(bool enable);
		bool IsShadowEnabled() const;

		void SetBackgroundColor(const glm::vec4& color);
		void SetBackgroundColor(const glm::vec3& color);
		glm::vec4 GetBackgroundColor() const;

		// ----- Properties -----
	  public:
		mutable std::mutex m_MutexState;
		std::string m_Text;
		glm::vec2 m_Position{0, 0};
		float m_TextHeight{16.f};
		Font::eTextAlignment m_TextAlignment{Font::eTextAlignment::Left};
		glm::vec4 m_TextColor{1, 1, 1, 1};
		glm::vec4 m_ShadowColor{0.246f, 0.246f, 0.246f, 1.f};
		glm::vec4 m_BackgroundColor{0.f, 0.f, 0.f, 0.f};
		std::atomic_bool m_ShadowEnabled{true};
		float m_zOffset{0.5f};
		float m_RotationDegrees{0.f};

		bool m_DefaultShadowColor = true;
	};
}; // namespace onion::voxel
