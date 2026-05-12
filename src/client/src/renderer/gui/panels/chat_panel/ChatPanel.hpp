#pragma once

#include <memory>
#include <vector>

#include <renderer/gui/GuiElement.hpp>
#include <renderer/gui/controls/scroller/Scroller.hpp>
#include <renderer/gui/controls/text_field/TextField.hpp>
#include <renderer/gui/panels/hud_panel/ChatTile.hpp>
#include <shared/chat_history/ChatMessage.hpp>

namespace onion::voxel
{
	class ChatPanel : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		ChatPanel(const std::string& name);
		~ChatPanel() override;

		// ----- Public API -----
	  public:
		void Render() override;
		void Initialize() override;
		void Delete() override;
		void ReloadTextures() override;

		void FocusTextField();

		// ----- Public Events -----
	  public:
		Event<const GuiElement*> EvtRequestBackNavigation;
		Event<const std::string&> EvtChatMessageSent;

		// ----- Controls -----
	  private:
		TextField m_Chat_TextField;
		Scroller m_Chat_Scroller;

		// ----- Chat Tiles -----
	  private:
		std::vector<std::unique_ptr<ChatTile>> m_ChatTiles;
		std::shared_ptr<const ChatMessage> m_LastChatMessage;

		// ----- Internal Event Subscription and Handlers -----
	  private:
		std::vector<EventHandle> m_EventHandles;
		void SubscribeToControlEvents();

		void Handle_TextFieldTextChanged(const TextField& sender);
	};
} // namespace onion::voxel
