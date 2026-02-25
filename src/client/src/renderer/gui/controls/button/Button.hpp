#pragma once

#include "../../../texture/texture.hpp"
#include "../../GuiElement.hpp"
#include "../../nine_slice_sprite/NineSliceSprite.hpp"

#include <onion/Event.hpp>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace onion::voxel
{
	class Button : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		Button(const std::string& name);
		~Button();

		// ----- Public API -----
	  public:
		void Initialize() override;
		void Render() override;
		void Delete() override;

		// ----- Getters / Setters -----
	  public:
		void SetText(const std::string& text);
		std::string GetText() const;

		void SetSize(const glm::ivec2& size);
		glm::ivec2 GetSize() const;

		void SetPosition(const glm::ivec2& pos);
		glm::ivec2 GetPosition() const;

		bool IsEnabled() const;
		void SetEnabled(bool enabled);

		// ----- Events -----
	  public:
		Event<const Button&> OnClick;
		Event<const Button&> OnHoverEnter;
		Event<const Button&> OnHoverLeave;

	  private:
		void SubscribeToSpriteEvents();

		EventHandle m_HandleMouseDown;
		void HandleMouseDown(const NineSliceSprite& sprite);
		EventHandle m_HandleMouseUp;
		void HandleMouseUp(const NineSliceSprite& sprite);
		EventHandle m_HandleSpriteClick;
		void HandleSpriteClick(const NineSliceSprite& sprite);
		EventHandle m_HandleSpriteHoverEnter;
		void HandleSpriteHoverEnter(const NineSliceSprite& sprite);
		EventHandle m_HandleSpriteHoverLeave;
		void HandleSpriteHoverLeave(const NineSliceSprite& sprite);

		// ----- Properties -----
	  private:
		std::string m_Text;
		bool m_IsEnabled = true;

		glm::ivec2 m_Position{0, 0};
		glm::ivec2 m_Size{1, 1};
		bool m_IsPressed = false;
		float m_ScaleFactorOnClick = 0.95f;

		// ----- NineSliceSprites -----
	  private:
		NineSliceSprite m_NineSliceSprite_Basic;
		NineSliceSprite m_NineSliceSprite_Disabled;
		NineSliceSprite m_NineSliceSprite_Highlighted;

		// ----- Static Helpers -----
	  private:
		static std::filesystem::path GetSpritePath_Basic();
		static std::filesystem::path GetSpritePath_Disabled();
		static std::filesystem::path GetSpritePath_Highlighted();

		// ----- DEBUG -----
	  private:
		void RenderImGuiDebug();
	};
} // namespace onion::voxel
