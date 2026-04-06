#include "KeyBindsTile.hpp"

namespace onion::voxel
{
	KeyBindsTile::KeyBindsTile(const std::string& name, eAction action, Key key)
		: GuiElement(name), m_Action(action), m_Key(key), m_LabelAction(name), m_ButtonKey("ButtonKey"),
		  m_ButtonReset("ButtonReset")
	{
		SubscribeToControlEvents();

		m_LabelAction.SetTextAlignment(Font::eTextAlignment::Left);
		m_ButtonReset.SetText("Reset");

		m_DefaultKey = GetDefaultKeyForAction(action);
	}

	KeyBindsTile::~KeyBindsTile()
	{
		m_EventHandles.clear();
	}

	void KeyBindsTile::Render()
	{
		if (m_KeyCapturedThisFrame)
		{
			m_KeyCapturedThisFrame = false;
			m_IsCapturingKey = false;
		}

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
		const float keyButtonPosXRatio = 981.f / originalTileWidth;
		const int keyButtonPosX = static_cast<int>(round(leftX + m_Size.x * keyButtonPosXRatio));
		const float buttonWidthRatio = 301.f / originalTileWidth;
		const int buttonWidth = static_cast<int>(round(m_Size.x * buttonWidthRatio));
		std::string keyText = KeyToString(m_Key);
		if (m_IsCapturingKey)
			keyText = "...";
		if (m_Key == Key::Unknown)
			keyText = "Not Bound";
		m_ButtonKey.SetPosition({keyButtonPosX, m_Position.y});
		m_ButtonKey.SetSize({buttonWidth, buttonsHeight});
		m_ButtonKey.SetText(keyText);
		m_ButtonKey.Render();

		// ---- Render Reset Button ----
		bool enableResetButton = (m_Key != m_DefaultKey);
		const float resetButtonPosXRatio = 1250.f / originalTileWidth;
		const int resetButtonPosX = static_cast<int>(round(leftX + m_Size.x * resetButtonPosXRatio));
		const float resetButtonWidthRatio = 200.f / originalTileWidth;
		const int resetButtonWidth = static_cast<int>(round(m_Size.x * resetButtonWidthRatio));
		m_ButtonReset.SetPosition({resetButtonPosX, m_Position.y});
		m_ButtonReset.SetSize({resetButtonWidth, buttonsHeight});
		m_ButtonReset.SetEnabled(enableResetButton);
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

	bool KeyBindsTile::IsDefaultKey() const
	{
		return m_Key == m_DefaultKey;
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
		m_DefaultKey = GetDefaultKeyForAction(action);
	}

	eAction KeyBindsTile::GetAction() const
	{
		return m_Action;
	}

	bool KeyBindsTile::IsCapturingKey() const
	{
		return m_IsCapturingKey;
	}

	void KeyBindsTile::SubscribeToControlEvents()
	{
		m_EventHandles.push_back(
			m_ButtonKey.OnClick.Subscribe([this](const Button& sender) { Handle_ButtonKey_Click(sender); }));

		m_EventHandles.push_back(
			m_ButtonReset.OnClick.Subscribe([this](const Button& sender) { Handle_ButtonReset_Click(sender); }));
	}

	void KeyBindsTile::Handle_ButtonKey_Click(const Button& sender)
	{
		(void) sender;

		InputsManager& inputsManager = *EngineContext::Get().Inputs;
		m_EvtHandle_KeyPressed =
			inputsManager.EvtIntercptedKeyPressed.Subscribe([this](Key key) { Handle_KeyPressed(key); });
		m_IsCapturingKey = true;
	}

	void KeyBindsTile::Handle_ButtonReset_Click(const Button& sender)
	{
		(void) sender;

		SetKey(m_DefaultKey);
		EvtKeyBindChanged.Trigger(*this);
	}

	void KeyBindsTile::Handle_KeyPressed(Key key)
	{
		if (key == Key::Escape)
		{
			// Unbound the Key
			key = Key::Unknown;
		}

		SetKey(key);
		EvtKeyBindChanged.Trigger(*this);
		m_KeyCapturedThisFrame = true;
	}

} // namespace onion::voxel
