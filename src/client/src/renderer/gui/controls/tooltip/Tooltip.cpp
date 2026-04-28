#include "Tooltip.hpp"

namespace onion::voxel
{
	// -------- Constructor --------

	Tooltip::Tooltip(const std::string& name)
		: GuiElement(name), m_Background(name + "_Background", s_SpritePathBackground),
		  m_Frame(name + "_Frame", s_SpritePathFrame), m_Label(name + "_Label")
	{
		m_Label.SetTextAlignment(Font::eTextAlignment::Left);
	}

	// -------- Public API --------

	void Tooltip::Initialize()
	{
		m_Background.Initialize();
		m_Frame.Initialize();
		m_Label.Initialize();

		SetInitState(true);
	}

	void Tooltip::Render()
	{
		const int padding = 12 * s_GuiScale;

		m_Label.SetTextHeight(m_TextHeight);
		m_Label.SetZOffset(m_ZOffset + 2.f * m_DeltaZ);

		const glm::ivec2 textSize = m_Label.GetTextSize();
		const glm::ivec2 size = {textSize.x + 2 * padding, (int) m_TextHeight + 2 * padding};

		// m_Position is left-center; NineSliceSprite expects center
		const glm::ivec2 center = {(int) m_Position.x + size.x / 2, (int) m_Position.y};

		m_Background.SetPosition(center);
		m_Background.SetSize(size);
		m_Background.SetZOffset(m_ZOffset);
		m_Background.Render();

		m_Frame.SetPosition(center);
		m_Frame.SetSize(size);
		m_Frame.SetZOffset(m_ZOffset + m_DeltaZ);
		m_Frame.Render();

		// Label left-center = left inner edge, vertically centered on m_Position.y
		m_Label.SetPosition({m_Position.x + padding, m_Position.y});
		m_Label.Render();
	}

	void Tooltip::Delete()
	{
		m_Background.Delete();
		m_Frame.Delete();
		m_Label.Delete();

		SetDeletedState(true);
	}

	void Tooltip::ReloadTextures()
	{
		m_Background.ReloadTextures();
		m_Frame.ReloadTextures();
	}

	// -------- Getters / Setters --------

	void Tooltip::SetText(const std::string& text)
	{
		if (m_Label.GetText() == text)
			return;

		m_Label.SetText(text);
	}

	std::string Tooltip::GetText() const
	{
		return m_Label.GetText();
	}

	void Tooltip::SetPosition(const glm::vec2& position)
	{
		m_Position = position;
	}

	glm::vec2 Tooltip::GetPosition() const
	{
		return m_Position;
	}

	void Tooltip::SetTextHeight(float textHeight)
	{
		m_TextHeight = textHeight;
	}

	float Tooltip::GetTextHeight() const
	{
		return m_TextHeight;
	}

	void Tooltip::SetZOffset(float zOffset)
	{
		m_ZOffset = zOffset;
	}

	float Tooltip::GetZOffset() const
	{
		return m_ZOffset;
	}

	void Tooltip::SetDeltaZ(float deltaZ)
	{
		m_DeltaZ = deltaZ;
	}

	float Tooltip::GetDeltaZ() const
	{
		return m_DeltaZ;
	}

} // namespace onion::voxel
