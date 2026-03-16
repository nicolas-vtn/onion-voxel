#pragma once

#include <glm/glm.hpp>

#include <string>
#include <vector>

#include <onion/Event.hpp>

#include <renderer/OpenGL.hpp>

#include <renderer/gui/GuiElement.hpp>
#include <renderer/gui/controls/label/Label.hpp>
#include <renderer/gui/nine_slice_sprite/NineSliceSprite.hpp>
#include <renderer/texture/texture.hpp>

namespace onion::voxel
{
	class TextField : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		TextField(const std::string& name);
		~TextField();

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

		void SetPlaceholderText(const std::string& placeholderText);
		std::string GetPlaceholderText() const;

		/// @brief Sets the size of the button. (In Pixels)
		/// @param size The new size of the button in pixels.
		void SetSize(const glm::ivec2& size);
		glm::ivec2 GetSize() const;

		/// @brief Sets the position of the button. (In Pixels)
		/// @param pos The new position of the button in pixels.
		void SetPosition(const glm::ivec2& pos);
		glm::ivec2 GetPosition() const;

		bool IsReadOnly() const;
		void SetReadOnly(bool readOnly);

		// ----- Events -----
	  public:
		Event<const TextField&> OnTextChanged;

	  private:
		void SubscribeToSpriteEvents();

		EventHandle m_HandleMouseDown;
		void Handle_MouseDown(const NineSliceSprite& sprite);
		EventHandle m_HandleMouseUp;
		void Handle_MouseUp(const NineSliceSprite& sprite);
		EventHandle m_HandleSpriteClick;
		void Handle_SpriteClick(const NineSliceSprite& sprite);
		EventHandle m_HandleSpriteHoverEnter;
		void Handle_SpriteHoverEnter(const NineSliceSprite& sprite);
		EventHandle m_HandleSpriteHoverLeave;
		void Handle_SpriteHoverLeave(const NineSliceSprite& sprite);

		// ----- Inputs Handling -----
	  private:
		void SubscribeToInputsManagerEvents();
		void RegisterInputsToInputsManager();

		EventHandle m_HandleCharInput;
		void Handle_CharInput(const unsigned int& codepoint);
		unsigned int m_LastCharInput = 0;

		void Handle_KeyInputs();
		void Handle_CharInputs();

		int m_InputId_Backspace;
		int m_InputId_Enter;
		int m_InputId_Escape;

		void ValidateText();

		// ----- Properties -----
	  private:
		bool m_ReadOnly = false;
		bool m_IsActive = false;
		std::string m_PlaceholderText = "Enter text...";

		float m_TextScaleFactor = 0.4f;

		std::string m_Text;
		glm::ivec2 m_Position{0, 0};
		glm::ivec2 m_Size{1, 1};
		bool m_IsPressed = false;

		// ----- Label -----
	  private:
		Label m_Label;

		// ----- NineSliceSprites -----
	  private:
		NineSliceSprite m_NineSliceSprite_TextField;
		NineSliceSprite m_NineSliceSprite_TextFieldHighlighted;

		// ----- Static Helpers -----
	  private:
		static inline const std::filesystem::path s_SpritePathFromGui =
			GuiElement::s_BasePathGuiAssets / "sprites" / "widget" / "text_field.png";

		static inline const std::filesystem::path s_SpritePathFromGui_Highlighted =
			GuiElement::s_BasePathGuiAssets / "sprites" / "widget" / "text_field_highlighted.png";

		static inline constexpr glm::vec3 s_TextColor = glm::vec3(224.f / 255.f);
		static inline constexpr glm::vec3 s_PlaceholderTextColor = glm::vec3(85.f / 255.f);

		// ----- DEBUG -----
	  private:
		void RenderImGuiDebug();
	};
} // namespace onion::voxel
