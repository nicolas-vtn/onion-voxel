#pragma once

#include "../../GuiElement.hpp"
#include "../../controls/button/Button.hpp"
#include "../../controls/label/Label.hpp"

namespace onion::voxel
{
	class ResourcePacksPanel : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		ResourcePacksPanel(const std::string& name);
		~ResourcePacksPanel() override;

		// ----- Public API -----
	  public:
		void Render() override;
		void Initialize() override;
		void Delete() override;

		// ----- Public Events -----
	  public:
		Event<const GuiElement*> RequestBackNavigation;

		// ----- Controls -----
	  private:
		Label m_Title_Label;

		Button m_OpenPackFolder_Button;
		Button m_Done_Button;

		// ----- Internal Event Subscription and Handlers -----
	  private:
		std::vector<EventHandle> m_EventHandles;
		void SubscribeToControlEvents();

		void Handle_OpenPackFolder_Click(const Button& sender);
		void Handle_Done_Click(const Button& sender);
	};
} // namespace onion::voxel
