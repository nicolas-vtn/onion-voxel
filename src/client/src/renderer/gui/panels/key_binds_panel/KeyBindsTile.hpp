#pragma once

#include <string>

#include <glm/glm.hpp>

#include <onion/DateTime.hpp>
#include <onion/Event.hpp>

#include <renderer/gui/GuiElement.hpp>
#include <renderer/gui/controls/button/Button.hpp>
#include <renderer/gui/controls/label/Label.hpp>
#include <renderer/key_binds/KeyBinds.hpp>

namespace onion::voxel
{
	class KeyBindsTile : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		KeyBindsTile(const std::string& name, eAction action, Key key);
		~KeyBindsTile();

		// ----- Public API -----
	  public:
		void Render() override;
		void Initialize() override;
		void Delete() override;
		void ReloadTextures() override;
		void SetVisibility(const Visibility& visibility) override;
		bool IsDefaultKey() const;

		// ----- Public Events -----
	  public:
		Event<const KeyBindsTile&> EvtKeyBindChanged;

		// ----- Getters / Setters -----
	  public:
		void SetSize(const glm::vec2& size);
		glm::vec2 GetSize() const;

		void SetPosition(const glm::vec2& pos);
		glm::vec2 GetPosition() const;

		void SetKey(Key key);
		Key GetKey() const;

		void SetAction(eAction action);
		eAction GetAction() const;

		bool IsCapturingKey() const;

		// ----- Properties -----
	  private:
		glm::vec2 m_Position{0, 0};
		glm::vec2 m_Size{1, 1};

		bool m_HasBeenInitialized = false;

		eAction m_Action;
		Key m_Key;
		Key m_DefaultKey;

		bool m_IsCapturingKey = false;

		// ----- Private Helpers -----
	  private:
		// ----- Events Subscription and Handlers -----
	  private:
		std::vector<EventHandle> m_EventHandles;
		void SubscribeToControlEvents();

		void Handle_ButtonKey_Click(const Button& sender);
		void Handle_ButtonReset_Click(const Button& sender);

		EventHandle m_EvtHandle_KeyPressed;
		void Handle_KeyPressed(Key key);

		// ----- Components -----
	  private:
		Label m_LabelAction;
		Button m_ButtonKey;
		Button m_ButtonReset;
	};
}; // namespace onion::voxel
