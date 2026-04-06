#include "Label.hpp"

namespace onion::voxel
{
	Label::Label(const std::string& name) : GuiElement(name) {}

	Label::~Label() {}

	void Label::Render()
	{
		std::lock_guard lock(m_MutexState);

		if (m_Text.empty())
			return;

		if (!m_HasCustomTextColor)
		{
			std::string formatedText = Font::FormatText(m_Text, m_TextColor, m_TextFormat);

			s_TextFont.RenderText(
				formatedText, m_TextAlignment, m_Position, m_TextHeight, m_zOffset, m_RotationDegrees, m_ShadowEnabled);
		}
		else
		{
			glm::vec4 shadowColor{};
			if (m_HasCustomShadowColor)
				shadowColor = m_CustomShadowColor;
			else
				shadowColor = {
					m_CustomTextColor.r / 4, m_CustomTextColor.g / 4, m_CustomTextColor.b / 4, m_CustomTextColor.a};

			s_TextFont.RenderText(m_Text,
								  m_TextAlignment,
								  m_Position,
								  m_CustomTextColor,
								  shadowColor,
								  m_TextHeight,
								  m_TextFormat,
								  m_zOffset,
								  m_RotationDegrees,
								  m_ShadowEnabled);
		}
	}

	void Label::Initialize()
	{
		ReloadTextures();
		SetInitState(true);
	}

	void Label::Delete()
	{
		SetDeletedState(true);
	}

	void Label::ReloadTextures()
	{
		s_TextFont.Reload();
	}

	void Label::SetText(const std::string& text)
	{
		std::lock_guard lock(m_MutexState);
		m_Text = text;
	}

	std::string Label::GetText() const
	{
		std::lock_guard lock(m_MutexState);
		return m_Text;
	}

	void Label::SetPosition(const glm::vec2& pos)
	{
		std::lock_guard lock(m_MutexState);
		m_Position = pos;
	}

	glm::vec2 Label::GetPosition() const
	{
		std::lock_guard lock(m_MutexState);
		return m_Position;
	}

	void Label::SetTextHeight(float height)
	{
		std::lock_guard lock(m_MutexState);
		m_TextHeight = height;
	}

	float Label::GetTextHeight() const
	{
		std::lock_guard lock(m_MutexState);
		return m_TextHeight;
	}

	glm::ivec2 Label::GetTextSize() const
	{
		std::lock_guard lock(m_MutexState);
		return s_TextFont.MeasureText(m_Text, m_TextHeight);
	}

	void Label::SetTextAlignment(Font::eTextAlignment alignment)
	{
		std::lock_guard lock(m_MutexState);
		m_TextAlignment = alignment;
	}

	Font::eTextAlignment Label::GetTextAlignment() const
	{
		std::lock_guard lock(m_MutexState);
		return m_TextAlignment;
	}

	void Label::SetTextColor(Font::eColor color)
	{
		std::lock_guard lock(m_MutexState);
		m_TextColor = color;
		m_HasCustomTextColor = false;
	}

	Font::eColor Label::GetTextColor() const
	{
		std::lock_guard lock(m_MutexState);
		return m_TextColor;
	}

	void Label::SetCustomTextColor(const glm::vec3& color)
	{
		SetCustomTextColor(glm::vec4(color, 1.f));
	}

	void Label::SetCustomTextColor(const glm::vec4& color)
	{
		std::lock_guard lock(m_MutexState);
		m_CustomTextColor = color;
		m_HasCustomTextColor = true;
	}

	glm::vec4 Label::GetCustomTextColor() const
	{
		std::lock_guard lock(m_MutexState);
		return m_CustomTextColor;
	}

	void Label::SetCustomShadowColor(const glm::vec4& color)
	{
		std::lock_guard lock(m_MutexState);
		m_CustomShadowColor = color;
		m_HasCustomShadowColor = true;
	}

	void Label::SetCustomShadowColor(const glm::vec3& color)
	{
		SetCustomShadowColor(glm::vec4(color, 1.f));
	}

	void Label::ResetShadowColor()
	{
		std::lock_guard lock(m_MutexState);
		m_HasCustomShadowColor = false;
	}

	glm::vec4 Label::GetCustomShadowColor() const
	{
		std::lock_guard lock(m_MutexState);
		return m_CustomShadowColor;
	}

	void Label::SetZOffset(float zOffset)
	{
		std::lock_guard lock(m_MutexState);
		m_zOffset = zOffset;
	}

	float Label::GetZOffset() const
	{
		std::lock_guard lock(m_MutexState);
		return m_zOffset;
	}

	void Label::SetRotationDegrees(float rotationDegrees)
	{
		std::lock_guard lock(m_MutexState);
		m_RotationDegrees = rotationDegrees;
	}

	float Label::GetRotationDegrees() const
	{
		std::lock_guard lock(m_MutexState);
		return m_RotationDegrees;
	}

	void Label::EnableShadow(bool enable)
	{
		std::lock_guard lock(m_MutexState);
		m_ShadowEnabled = enable;
	}

	bool Label::IsShadowEnabled() const
	{
		std::lock_guard lock(m_MutexState);
		return m_ShadowEnabled;
	}

	void Label::SetTextFormat(const Font::TextFormat& textFormat)
	{
		std::lock_guard lock(m_MutexState);
		m_TextFormat = textFormat;
	}

	Font::TextFormat Label::GetTextFormat() const
	{
		std::lock_guard lock(m_MutexState);
		return m_TextFormat;
	}

} // namespace onion::voxel
