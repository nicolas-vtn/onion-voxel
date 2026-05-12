#include "ChatPanel.hpp"

#include <renderer/gui/colored_background/ColoredBackground.hpp>

namespace onion::voxel
{
	ChatPanel::ChatPanel(const std::string& name, ChatHistory& chatHistory)
		: GuiElement(name), m_Chat_TextField(name + "_Chat_TextField"), m_ChatHistory(chatHistory)
	{
		SubscribeToControlEvents();

		m_Chat_TextField.SetPlaceholderText("");
		m_Chat_TextField.SetValidateOnlyOnEnter(true);
		m_Chat_TextField.SetClearOnRightClick(false);
		m_Chat_TextField.SetRenderSprites(false);
		m_Chat_TextField.SetZOffset(0.6f);
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

		const int marginX = static_cast<int>(round(s_ScreenWidth * marginXRatio));
		const int fieldWidth = static_cast<int>(round(s_ScreenWidth + (4 * marginX)));

		const int fieldX = fieldWidth / 2;
		const int fieldY = s_ScreenHeight - (s_ControlHeight / 2);

		// ----- Chat TextField Background -----
		int topTxtFieldY = fieldY - (s_ControlHeight / 3);	  // Smaller than TextField
		int bottomTxtFieldY = fieldY + (s_ControlHeight / 3); // Smaller than TextField
		int leftTxtFieldX = fieldX - (fieldWidth / 2) + (3 * marginX);
		int rightTxtFieldX = fieldX + (fieldWidth / 2) - (3 * marginX);
		glm::vec2 topLeftTxtField{leftTxtFieldX, topTxtFieldY};
		glm::vec2 bottomRightTxtField{rightTxtFieldX, bottomTxtFieldY};

		ColoredBackground::CornerOptions bgOptions;
		bgOptions.TopLeftCorner = topLeftTxtField;
		bgOptions.BottomRightCorner = bottomRightTxtField;
		bgOptions.Color = glm::vec4(0, 0, 0, 0.5f);
		bgOptions.ZOffset = 0.5f; // Beheind m_Chat_TextField

		ColoredBackground::Render(bgOptions);

		// ----- Render Chat TextField (bottom-left) -----
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
