#pragma once

#include <renderer/gui/GuiElement.hpp>
#include <renderer/gui/controls/sprite/Sprite.hpp>

namespace onion::voxel
{
	class InventoryPanel : public GuiElement
	{
		// ----- Constructor / Destructor -----
	  public:
		InventoryPanel(const std::string& name);
		~InventoryPanel() override;

		// ----- Public API -----
	  public:
		void Render() override;
		void Initialize() override;
		void Delete() override;
		void ReloadTextures() override;

		// ----- Public Events -----
	  public:
		Event<const GuiElement*> EvtRequestBackNavigation;

		// ----- Controls -----
	  private:
		Sprite m_InventoryBackground_Sprite;

		// ----- Textures -----
	  private:
		static inline const std::filesystem::path s_PathInventoryBackground =
			GuiElement::s_BasePathGuiAssets / "container" / "inventory.png";

		// ----- Internal Event Subscription and Handlers -----
	  private:
		std::vector<EventHandle> m_EventHandles;
		void SubscribeToControlEvents();
	};
} // namespace onion::voxel
