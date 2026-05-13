#include "ChatPanel.hpp"

#include <renderer/EngineContext.hpp>
#include <renderer/gui/colored_background/ColoredBackground.hpp>

namespace onion::voxel
{
	ChatPanel::ChatPanel(const std::string& name)
		: GuiElement(name), m_Chat_TextField(name + "_Chat_TextField"), m_Chat_Scroller(name + "_Chat_Scroller")
	{
		SubscribeToControlEvents();

		m_Chat_TextField.SetPlaceholderText("");
		m_Chat_TextField.SetValidateOnlyOnEnter(true);
		m_Chat_TextField.SetClearOnRightClick(false);
		m_Chat_TextField.SetRenderSprites(false);
		m_Chat_TextField.SetZOffset(0.6f);

		m_Chat_Scroller.SetRenderBorders(false);
		m_Chat_Scroller.SetRenderBackground(false);
		m_Chat_Scroller.SetHandleXPositionRatio(1.f);
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

#ifndef NDEBUG
		bool debugDummyHistory = false;
		if (debugDummyHistory && m_ChatTiles.size() == 0)
		{
			// Populate Tiles with Debug History
			std::shared_ptr<Player> player = EngineContext::Get().GetLocalPlayer();
			for (int i = 0; i < 30; i++)
			{
				std::string content = "This is a debug message #" + std::to_string(i);
				auto dummyMessage =
					std::make_shared<ChatMessage>(DateTime::UtcNow(), player->GetName(), player->UUID, content);
				auto tile = std::make_unique<ChatTile>("ChatPanelTile_Debug_" + std::to_string(i), dummyMessage);
				tile->Initialize();
				m_ChatTiles.push_back(std::move(tile));
			}
		}
#endif // DEBUG

		// ----- Exit Conditions -----
		if (IsBackPressed())
		{
			EvtRequestBackNavigation.Trigger(this);
			return;
		}

		// Force focus even when clicked outside
		m_Chat_TextField.SetActive(true);

		// Tile/scroller width matches the ChatTile background width exactly.
		const float bgWidthRatio = 1328.f / 1920.f;
		const int tileWidth = static_cast<int>(s_ScreenWidth * bgWidthRatio);

		// Anchor: same absolute Y as HudPanel's chat tile anchor.
		const float cursorYRatio = (875.f - 23.f) / 1009.f;
		const int scrollerBottomY = static_cast<int>(cursorYRatio * s_ScreenHeight);

		const int scrollerHeight = static_cast<int>(round(s_ScreenHeight * 0.7f));

		const glm::vec2 topLeftScroller{0, scrollerBottomY - scrollerHeight};
		const glm::vec2 bottomRightScroller{tileWidth, scrollerBottomY};

		// ----- Update scroll area height before any SetScrollRatio call -----
		// Must happen early so that SetScrollRatio(1.f) below resolves the correct offset.
		{
			int totalTilesHeight = 0;
			for (const auto& tile : m_ChatTiles)
				totalTilesHeight += tile->GetSize().y;
			m_Chat_Scroller.SetScrollAreaHeight(static_cast<uint32_t>(std::max(scrollerHeight, totalTilesHeight)));
		}

		if (IsFirstFrameAfterPanelChange())
		{
			// Reset text, prevents inserting 't'.
			m_Chat_TextField.SetText("");
			m_Chat_Scroller.SetScrollRatio(1.f);
		}

		// ----- Render Text Field Background -----
		const float bgLeftXposRatio = 8.f / 1920.f;
		const int bgLeftXpos = static_cast<int>(s_ScreenWidth * bgLeftXposRatio);
		const float bgRightXposRatio = 1912.f / 1920.f;
		const int bgRightXpos = static_cast<int>(s_ScreenWidth * bgRightXposRatio);
		const float bgTopYposRatio = (979.f - 23.f) / 1009.f;
		const int bgTopYpos = static_cast<int>(s_ScreenHeight * bgTopYposRatio);
		const float bgBottomYposRatio = 1004.f / 1009.f;
		const int bgBottomYpos = static_cast<int>(s_ScreenHeight * bgBottomYposRatio);

		ColoredBackground::CornerOptions bgOptions;
		bgOptions.TopLeftCorner = {bgLeftXpos, bgTopYpos};
		bgOptions.BottomRightCorner = {bgRightXpos, bgBottomYpos};
		bgOptions.Color = glm::vec4(0, 0, 0, 0.5f);
		bgOptions.ZOffset = 0.5f;
		ColoredBackground::Render(bgOptions);

		// ----- Render Text Field -----
		const float fieldYposRatio = (1003 - 23) / 1009.f;
		const int fieldYpos = static_cast<int>(s_ScreenHeight * fieldYposRatio);
		const float fieldXposRatio = 939 / 1920.f;
		const int fieldXpos = static_cast<int>(s_ScreenWidth * fieldXposRatio);
		m_Chat_TextField.SetPosition({fieldXpos, fieldYpos});
		m_Chat_TextField.SetSize({s_ScreenWidth, s_ControlHeight});
		m_Chat_TextField.Render();

		// ----- Sync tiles with history (same algorithm as HudPanel) -----
		if (EngineContext::Get().Chat != nullptr)
		{
			const auto history = EngineContext::Get().Chat->GetReceivedHistory();

			size_t newCount = history.size();
			if (m_LastChatMessage != nullptr)
			{
				for (size_t i = 0; i < history.size(); ++i)
				{
					if (history[i] == m_LastChatMessage)
					{
						newCount = i;
						break;
					}
				}
			}

			for (size_t i = newCount; i > 0; i--)
			{
				auto tile =
					std::make_unique<ChatTile>("ChatPanelTile_" + std::to_string(m_ChatTiles.size()), history[i - 1]);
				tile->Initialize();
				m_ChatTiles.insert(m_ChatTiles.begin(), std::move(tile));
			}

			if (!history.empty())
				m_LastChatMessage = history.front();
		}

		// ----- Render Scroller -----
		m_Chat_Scroller.SetTopLeftCorner(topLeftScroller);
		m_Chat_Scroller.SetBottomRightCorner(bottomRightScroller);
		m_Chat_Scroller.Render();

		// ----- Render Chat Tiles inside scroller -----
		m_Chat_Scroller.StartCissoring();

		// Compute total tiles height (reused for anchor offset).
		int totalTilesHeight = 0;
		for (const auto& tile : m_ChatTiles)
			totalTilesHeight += tile->GetSize().y;

		const int viewportHeight = static_cast<int>(bottomRightScroller.y - topLeftScroller.y);

		// Lay tiles bottom-up from the scroller bottom, same layout as HudPanel.
		// m_ChatTiles[0] is newest (sits lowest).
		// Pre-offset the raw anchor so that at scrollRatio=1 the bottom of the newest tile
		// lands exactly at scrollerBottomY, regardless of total content height.
		const int scrollableOverflow = std::max(0, totalTilesHeight - viewportHeight);
		float tileY = static_cast<float>(scrollerBottomY + scrollableOverflow);
		for (const auto& tile : m_ChatTiles)
		{
			const glm::ivec2 projectedPos = m_Chat_Scroller.ProjectContentPosition({0, static_cast<int>(tileY)});
			tile->SetPosition({0.f, static_cast<float>(projectedPos.y)});
			tile->Render(false);

			tileY -= static_cast<float>(tile->GetSize().y);
		}

		m_Chat_Scroller.StopCissoring();
	}

	void ChatPanel::Initialize()
	{
		m_Chat_TextField.Initialize();
		m_Chat_Scroller.Initialize();

		SetInitState(true);
	}

	void ChatPanel::Delete()
	{
		m_Chat_TextField.Delete();
		m_Chat_Scroller.Delete();

		for (auto& tile : m_ChatTiles)
			tile->Delete();
		m_ChatTiles.clear();

		SetDeletedState(true);
	}

	void ChatPanel::ReloadTextures()
	{
		m_Chat_TextField.ReloadTextures();
		m_Chat_Scroller.ReloadTextures();

		for (auto& tile : m_ChatTiles)
			tile->ReloadTextures();
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
