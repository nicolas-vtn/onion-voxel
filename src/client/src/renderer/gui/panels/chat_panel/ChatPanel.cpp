#include "ChatPanel.hpp"

#include <ranges>

#include <renderer/gui/colored_background/ColoredBackground.hpp>

namespace onion::voxel
{
	ChatPanel::ChatPanel(const std::string& name, ChatHistory& chatHistory)
		: GuiElement(name), m_Chat_TextField(name + "_Chat_TextField"), m_Chat_Scroller(name + "_Chat_Scroller"),
		  m_ChatHistory_Label(name + "_ChatHistory_Label"), m_ChatHistory(chatHistory)
	{
		SubscribeToControlEvents();

		m_Chat_TextField.SetPlaceholderText("");
		m_Chat_TextField.SetValidateOnlyOnEnter(true);
		m_Chat_TextField.SetClearOnRightClick(false);
		m_Chat_TextField.SetRenderSprites(false);
		m_Chat_TextField.SetZOffset(0.6f);

		m_Chat_Scroller.SetRenderBorders(false);
		m_Chat_Scroller.SetHandleXPositionRatio(1.f);

		m_ChatHistory_Label.SetTextAlignment(Font::eTextAlignment::Left);
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
			m_Chat_Scroller.SetScrollRatio(1.f);
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
		const int topTxtFieldY = fieldY - (s_ControlHeight / 3);	// Smaller than TextField
		const int bottomTxtFieldY = fieldY + (s_ControlHeight / 3); // Smaller than TextField
		const int leftTxtFieldX = fieldX - (fieldWidth / 2) + (3 * marginX);
		const int rightTxtFieldX = fieldX + (fieldWidth / 2) - (3 * marginX);
		const glm::vec2 topLeftTxtField{leftTxtFieldX, topTxtFieldY};
		const glm::vec2 bottomRightTxtField{rightTxtFieldX, bottomTxtFieldY};

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

		// ----- Render Chat Scroller -----
		const int scrollerWidth = static_cast<int>(round(s_ScreenWidth * 0.6f));
		const int scrollerHeight = static_cast<int>(round(s_ScreenHeight * 0.7f));
		const int scrollerLeftX = leftTxtFieldX;
		const int scrollerBottomY = topTxtFieldY - (2 * marginX);

		const glm::vec2 topLeftScroller{scrollerLeftX, scrollerBottomY - scrollerHeight};
		const glm::vec2 bottomRightScroller{scrollerLeftX + scrollerWidth, scrollerBottomY};

		m_Chat_Scroller.SetTopLeftCorner(topLeftScroller);
		m_Chat_Scroller.SetBottomRightCorner(bottomRightScroller);
		m_Chat_Scroller.Render();

		// ----- Render Chat Messages -----
		m_Chat_Scroller.StartCissoring();

		const auto messages = m_ChatHistory.GetReceivedHistory();
		std::string fullChat;

		for (const auto& message : std::views::reverse(messages))
		{
			fullChat += "<" + message->PlayerName + "> " + message->Content + "\n";
		}

		//for (int i = 0; i < 30; i++) // Simulate 30 messages to test scrolling
		//	fullChat += "<PlayerName> " + std::to_string(i) + "\n";

		m_ChatHistory_Label.SetText(fullChat);
		m_ChatHistory_Label.SetTextHeight(s_TextHeight);

		const glm::vec2 textSize = m_ChatHistory_Label.GetTextSize();
		const int textX = scrollerLeftX + marginX;
		const int viewportHeight = static_cast<int>(bottomRightScroller.y - topLeftScroller.y);

		const int textHeight = static_cast<int>(round(textSize.y));
		const int textY = (textHeight >= viewportHeight)
			? scrollerBottomY + (5 * marginX) + textHeight / 2 - viewportHeight // scrollable: anchor bottom at ratio=1
			: scrollerBottomY + (5 * marginX) - textHeight / 2;					// fits: anchor bottom directly

		glm::vec2 chatHistoryPosition{textX, textY};
		chatHistoryPosition = m_Chat_Scroller.ProjectContentPosition(chatHistoryPosition);
		m_ChatHistory_Label.SetPosition(chatHistoryPosition);
		m_ChatHistory_Label.Render();

		const int scrolAreaHeight = static_cast<int>(std::max(bottomRightScroller.y - topLeftScroller.y, textSize.y));
		m_Chat_Scroller.SetScrollAreaHeight(scrolAreaHeight);

		m_Chat_Scroller.StopCissoring();
	}

	void ChatPanel::Initialize()
	{
		m_Chat_TextField.Initialize();
		m_Chat_Scroller.Initialize();
		m_ChatHistory_Label.Initialize();

		SetInitState(true);
	}

	void ChatPanel::Delete()
	{
		m_Chat_TextField.Delete();
		m_Chat_Scroller.Delete();
		m_ChatHistory_Label.Delete();

		SetDeletedState(true);
	}

	void ChatPanel::ReloadTextures()
	{
		m_Chat_TextField.ReloadTextures();
		m_Chat_Scroller.ReloadTextures();
		m_ChatHistory_Label.ReloadTextures();
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
