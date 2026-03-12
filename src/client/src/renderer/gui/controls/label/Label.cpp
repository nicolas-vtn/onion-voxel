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

		if (m_ShadowEnabled)
		{
			float shadowOffset = m_TextHeight / s_TextFont.GetGlyphSize().y;
			glm::vec2 shadowOffsetVec{shadowOffset, shadowOffset};

			s_TextFont.RenderText(m_Text,
								  m_TextAlignment,
								  m_Position + shadowOffsetVec,
								  m_TextHeight,
								  m_ShadowColor,
								  m_zOffset - 0.01f,
								  m_RotationDegrees);
		}

		s_TextFont.RenderText(
			m_Text, m_TextAlignment, m_Position, m_TextHeight, m_TextColor, m_zOffset, m_RotationDegrees);
	}

	void Label::Initialize()
	{
		SetInitState(true);
	}

	void Label::Delete()
	{
		SetDeletedState(true);
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

	void Label::SetTextColor(const glm::vec4& color)
	{
		std::lock_guard lock(m_MutexState);
		m_TextColor = color;
	}

	glm::vec4 Label::GetTextColor() const
	{
		std::lock_guard lock(m_MutexState);
		return m_TextColor;
	}

	void Label::SetShadowColor(const glm::vec4& color)
	{
		std::lock_guard lock(m_MutexState);
		m_ShadowColor = color;
	}

	glm::vec4 Label::GetShadowColor() const
	{
		std::lock_guard lock(m_MutexState);
		return m_ShadowColor;
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

} // namespace onion::voxel
