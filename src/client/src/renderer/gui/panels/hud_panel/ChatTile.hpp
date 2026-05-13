#pragma once

#include <memory>
#include <string>

#include <glm/glm.hpp>

#include <renderer/gui/GuiElement.hpp>
#include <renderer/gui/controls/label/Label.hpp>
#include <shared/chat_history/ChatMessage.hpp>

namespace onion::voxel
{
	class ChatTile : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		ChatTile(const std::string& name, std::shared_ptr<const ChatMessage> message);
		~ChatTile() override = default;

		// ----- Public API -----
	  public:
		void Render() override;
		void Render(bool shouldFade);
		void Initialize() override;
		void Delete() override;
		void ReloadTextures() override;

		// ----- Getters / Setters -----
	  public:
		// Sets the position of the bottom-left corner of the tile.
		void SetPosition(const glm::vec2& pos);
		glm::vec2 GetPosition() const;

		// Returns the pixel size of the tile (width x height), measured from the label text.
		glm::ivec2 GetSize() const;

		// Returns the glfwGetTime() value captured when this tile was created.
		double GetSpawnTime() const;

		// Returns the current opacity of the tile. 0 means fully faded (safe to destroy).
		float GetFadingAlpha() const;

		// ----- Private Helpers -----
	  private:
		// Seconds before the tile starts fading out.
		static constexpr double k_OpaqueSeconds = 10.0;
		// Duration of the fade-out transition in seconds.
		static constexpr double k_FadeSeconds = 1.0;

		// ----- Properties -----
	  private:
		glm::vec2 m_Position{0, 0};
		double m_SpawnTime{0.0};
		float m_FadingAlpha{1.f};
		std::shared_ptr<const ChatMessage> m_Message;

		// ----- Components -----
	  private:
		Label m_Label;
	};
} // namespace onion::voxel
