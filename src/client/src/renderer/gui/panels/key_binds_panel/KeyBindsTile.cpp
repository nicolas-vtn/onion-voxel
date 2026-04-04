#include "KeyBindsTile.hpp"

namespace onion::voxel
{
	KeyBindsTile::KeyBindsTile(const std::string& name, eAction action, Key key)
		: GuiElement(name), m_Action(action), m_Key(key), m_LabelAction(name), m_ButtonKey("ButtonKey"),
		  m_ButtonReset("ButtonReset")
	{
		m_LabelAction.SetTextAlignment(Font::eTextAlignment::Left);
		m_ButtonReset.SetText("Reset");
	}

	KeyBindsTile::~KeyBindsTile()
	{
		m_EventHandles.clear();
	}

	void KeyBindsTile::Render()
	{
		// ---- Constants ----
		const int leftX = static_cast<int>(round(m_Position.x - m_Size.x / 2.f));
		const int buttonsHeight = static_cast<int>(round(m_Size.y));

		// ---- Render Action Label ----
		const float textHeightRatio = 32.f / 1009.f;
		const float textHeight = s_ScreenHeight * textHeightRatio;
		const std::string actionText = ActionToString(m_Action);
		m_LabelAction.SetPosition({leftX, m_Position.y});
		m_LabelAction.SetTextHeight(textHeight);
		m_LabelAction.SetText(actionText);
		m_LabelAction.Render();

		//---- Render Key Button ----
		const float originalTileWidth = 1355.f;
		const float keyButtonPosXRatio = 980.f / originalTileWidth;
		const int keyButtonPosX = static_cast<int>(round(leftX + m_Size.x * keyButtonPosXRatio));
		const float buttonWidthRatio = 300.f / originalTileWidth;
		const int buttonWidth = static_cast<int>(round(m_Size.x * buttonWidthRatio));
		const std::string keyText = KeyToString(m_Key);
		m_ButtonKey.SetPosition({keyButtonPosX, m_Position.y});
		m_ButtonKey.SetSize({buttonWidth, buttonsHeight});
		m_ButtonKey.SetText(keyText);
		m_ButtonKey.Render();

		// ---- Render Reset Button ----
		const float resetButtonPosXRatio = 1250.f / originalTileWidth;
		const int resetButtonPosX = static_cast<int>(round(leftX + m_Size.x * resetButtonPosXRatio));
		const float resetButtonWidthRatio = 200.f / originalTileWidth;
		const int resetButtonWidth = static_cast<int>(round(m_Size.x * resetButtonWidthRatio));
		m_ButtonReset.SetPosition({resetButtonPosX, m_Position.y});
		m_ButtonReset.SetSize({resetButtonWidth, buttonsHeight});
		m_ButtonReset.Render();
	}

	void KeyBindsTile::Initialize()
	{
		m_LabelAction.Initialize();
		m_ButtonKey.Initialize();
		m_ButtonReset.Initialize();

		SetInitState(true);
	}

	void KeyBindsTile::Delete()
	{
		m_LabelAction.Delete();
		m_ButtonKey.Delete();
		m_ButtonReset.Delete();

		SetDeletedState(true);
	}

	void KeyBindsTile::ReloadTextures()
	{
		m_LabelAction.ReloadTextures();
		m_ButtonKey.ReloadTextures();
		m_ButtonReset.ReloadTextures();
	}

	void KeyBindsTile::SetVisibility(const Visibility& visibility)
	{
		Visibility btKeyVisibility = Visibility::Compose(visibility, m_ButtonKey.GetPosition(), m_ButtonKey.GetSize());
		m_ButtonKey.SetVisibility(btKeyVisibility);

		Visibility btResetVisibility =
			Visibility::Compose(visibility, m_ButtonReset.GetPosition(), m_ButtonReset.GetSize());
		m_ButtonReset.SetVisibility(btResetVisibility);
	}

	void KeyBindsTile::SetSize(const glm::vec2& size)
	{
		m_Size = size;
	}

	glm::vec2 KeyBindsTile::GetSize() const
	{
		return m_Size;
	}

	void KeyBindsTile::SetPosition(const glm::vec2& pos)
	{
		m_Position = pos;
	}

	glm::vec2 KeyBindsTile::GetPosition() const
	{
		return m_Position;
	}

	void KeyBindsTile::SetKey(Key key)
	{
		if (m_Key == key)
			return;

		m_Key = key;
	}

	Key KeyBindsTile::GetKey() const
	{
		return m_Key;
	}

	void KeyBindsTile::SetAction(eAction action)
	{
		m_Action = action;
	}

	eAction KeyBindsTile::GetAction() const
	{
		return m_Action;
	}

} // namespace onion::voxel
