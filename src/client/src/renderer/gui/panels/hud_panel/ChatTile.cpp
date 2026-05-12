#include "ChatTile.hpp"

#include <stdexcept>

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
		throw std::logic_error("Use Render(bool shouldFade) instead of Render() for ChatTile.");
	}

	void ChatTile::Render(bool shouldFade)
	{
		// ---- Compute fading alpha from tile age ----
		if (!shouldFade)
		{
			m_FadingAlpha = 1.f;
		}
		else
		{
			const double age = glfwGetTime() - m_SpawnTime;
			if (age >= k_OpaqueSeconds + k_FadeSeconds)
				m_FadingAlpha = 0.f;
			else if (age > k_OpaqueSeconds)
				m_FadingAlpha = 1.f - static_cast<float>((age - k_OpaqueSeconds) / k_FadeSeconds);
			else
				m_FadingAlpha = 1.f;
		}

		m_Label.SetTextHeight(s_TextHeight);
		const glm::ivec2 textSize = m_Label.GetTextSize();
		const int bgPaddingTop = static_cast<int>(s_ScreenHeight * (4.f / 1009.f));

		const float bgWidthRatio = 1328.f / 1920.f;
		const int bgWidth = static_cast<int>(s_ScreenWidth * bgWidthRatio);

		// Draw background first, behind the text.
		// m_Position is the bottom-left corner; Y increases downward, so top is at (y - textSize.y).
		// bgPaddingTop extends the background upward; bgPaddingBottom extends it downward.
		const int leftX = static_cast<int>(m_Position.x);
		const int rightX = leftX + bgWidth;
		const int tileTopY = static_cast<int>(m_Position.y) - textSize.y - bgPaddingTop;
		const int tileBottomY = static_cast<int>(m_Position.y);

		ColoredBackground::CornerOptions options;
		options.TopLeftCorner = {leftX, tileTopY};
		options.BottomRightCorner = {rightX, tileBottomY};
		options.Color = glm::vec4{0.f, 0.f, 0.f, 0.5f * m_FadingAlpha};
		ColoredBackground::Render(options);

		const float textPaddingXratio = 16.f / 1920.f;
		const int textPaddingX = static_cast<int>(s_ScreenWidth * textPaddingXratio);

		// Label position is its center; derive it from the bottom-left corner and the text size.
		m_Label.SetCustomTextColor({1.f, 1.f, 1.f, m_FadingAlpha});
		m_Label.SetPosition({m_Position.x + textPaddingX, m_Position.y - textSize.y / 2.f});
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
