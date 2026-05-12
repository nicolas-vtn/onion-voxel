#pragma once

#include <renderer/gui/GuiElement.hpp>
#include <renderer/gui/controls/label/Label.hpp>
#include <renderer/gui/controls/scroller/Scroller.hpp>
#include <renderer/gui/controls/text_field/TextField.hpp>

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
		Label m_ChatHistory_Label;

		// ----- Internal Event Subscription and Handlers -----
	  private:
		std::vector<EventHandle> m_EventHandles;
		void SubscribeToControlEvents();

		void Handle_TextFieldTextChanged(const TextField& sender);
	};
} // namespace onion::voxel
