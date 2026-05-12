#include "ChatTile.hpp"

#include <GLFW/glfw3.h>

#include <renderer/gui/colored_background/ColoredBackground.hpp>

namespace onion::voxel
{
	ChatTile::ChatTile(const std::string& name, std::shared_ptr<const ChatMessage> message)
		: GuiElement(name), m_Message(message), m_SpawnTime(glfwGetTime()), m_Label(name + "_Label")
	{
		const std::string labelText = "<" + message->PlayerName + "> " + message->Content;
		m_Label.SetText(labelText);
		m_Label.SetTextAlignment(Font::eTextAlignment::Left);
	}

	void ChatTile::Initialize()
	{
		m_Label.Initialize();
		SetInitState(true);
	}

	void ChatTile::Delete()
	{
		m_Label.Delete();
		SetDeletedState(true);
	}

	void ChatTile::ReloadTextures()
	{
		m_Label.ReloadTextures();
	}

	void ChatTile::Render()
	{
		// ---- Compute fading alpha from tile age ----
		const double age = glfwGetTime() - m_SpawnTime;
		if (age >= k_OpaqueSeconds + k_FadeSeconds)
			m_FadingAlpha = 0.f;
		else if (age > k_OpaqueSeconds)
			m_FadingAlpha = 1.f - static_cast<float>((age - k_OpaqueSeconds) / k_FadeSeconds);
		else
			m_FadingAlpha = 1.f;

		m_Label.SetTextHeight(s_TextHeight);
		const glm::ivec2 textSize = m_Label.GetTextSize();
		const int bgPaddingTop = static_cast<int>(s_ScreenHeight * (4.f / 1009.f));

		// Draw background first, behind the text.
		// m_Position is the bottom-left corner; Y increases downward, so top is at (y - textSize.y).
		// bgPaddingTop extends the background upward; bgPaddingBottom extends it downward.
		ColoredBackground::Render(ColoredBackground::CornerOptions{
			.TopLeftCorner = {static_cast<int>(m_Position.x),
							  static_cast<int>(m_Position.y) - textSize.y - bgPaddingTop},
			.BottomRightCorner = {static_cast<int>(m_Position.x) + textSize.x, static_cast<int>(m_Position.y)},
			.Color = {0.f, 0.f, 0.f, 0.5f * m_FadingAlpha},
		});

		// Label position is its center; derive it from the bottom-left corner and the text size.
		m_Label.SetCustomTextColor({1.f, 1.f, 1.f, m_FadingAlpha});
		m_Label.SetPosition({m_Position.x, m_Position.y - textSize.y / 2.f});
		m_Label.Render();
	}

	void ChatTile::SetPosition(const glm::vec2& pos)
	{
		m_Position = pos;
	}

	glm::vec2 ChatTile::GetPosition() const
	{
		return m_Position;
	}

	glm::ivec2 ChatTile::GetSize() const
	{
		const glm::ivec2 textSize = m_Label.GetTextSize();
		const int bgPaddingTop = static_cast<int>(s_ScreenHeight * (4.f / 1009.f));
		return {textSize.x, textSize.y + bgPaddingTop};
	}

	double ChatTile::GetSpawnTime() const
	{
		return m_SpawnTime;
	}

	float ChatTile::GetFadingAlpha() const
	{
		return m_FadingAlpha;
	}
} // namespace onion::voxel
