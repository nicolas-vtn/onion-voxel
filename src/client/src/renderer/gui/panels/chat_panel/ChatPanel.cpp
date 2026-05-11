#include "ChatPanel.hpp"

namespace onion::voxel
{
	ChatPanel::ChatPanel(const std::string& name) : GuiElement(name), m_Chat_TextField(name + "_Chat_TextField")
	{
		SubscribeToControlEvents();

		m_Chat_TextField.SetPlaceholderText("");
		m_Chat_TextField.SetValidateOnlyOnEnter(true);
	}

	ChatPanel::~ChatPanel()
	{
		m_EventHandles.clear();
	}

	void ChatPanel::FocusTextField()
	{
		m_Chat_TextField.SetText("");
		m_Chat_TextField.SetActive(true);
	}

	void ChatPanel::Render()
	{
		// ----- Exit Conditions -----
		if (IsBackPressed())
		{
			EvtRequestBackNavigation.Trigger(this);
			return;
		}

		if (!AreKeyInputsValid())
		{
			// Reset text, prevents to insert 't'.
			m_Chat_TextField.SetText("");
		}

		// Force focus even when clicked outside
		m_Chat_TextField.SetActive(true);

		// ----- Constants -----
		const float marginXRatio = 8.f / 1920.f;
		const float marginYRatio = 8.f / 1009.f;
		const float fieldWidthRatio = 680.f / 1920.f;

		const int marginX = static_cast<int>(round(s_ScreenWidth * marginXRatio));
		const int marginY = static_cast<int>(round(s_ScreenHeight * marginYRatio));
		const int fieldWidth = static_cast<int>(round(s_ScreenWidth * fieldWidthRatio));

		// ----- Render Chat TextField (bottom-left) -----
		const int fieldX = marginX + (fieldWidth / 2);
		const int fieldY = s_ScreenHeight - marginY - (s_ControlHeight / 2);

		m_Chat_TextField.SetPosition({fieldX, fieldY});
		m_Chat_TextField.SetSize({fieldWidth, s_ControlHeight});
		m_Chat_TextField.Render();
	}

	void ChatPanel::Initialize()
	{
		m_Chat_TextField.Initialize();
		SetInitState(true);
	}

	void ChatPanel::Delete()
	{
		m_Chat_TextField.Delete();
		SetDeletedState(true);
	}

	void ChatPanel::ReloadTextures()
	{
		m_Chat_TextField.ReloadTextures();
	}

	void ChatPanel::SubscribeToControlEvents()
	{
		m_EventHandles.push_back(m_Chat_TextField.EvtTextValidated.Subscribe([this](const TextField& sender)
																			 { Handle_TextFieldTextChanged(sender); }));
	}

	void ChatPanel::Handle_TextFieldTextChanged(const TextField& sender)
	{
		std::string message = sender.GetText();

		if (message.empty())
			return;

		m_Chat_TextField.SetText("");
		EvtChatMessageSent.Trigger(message);
		EvtRequestBackNavigation.Trigger(this);
	}

} // namespace onion::voxel
