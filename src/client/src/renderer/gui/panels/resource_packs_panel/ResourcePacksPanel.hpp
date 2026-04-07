#pragma once

#include <mutex>
#include <string>
#include <vector>

#include <onion/ThreadSafeQueue.hpp>
#include <onion/Timer.hpp>

#include <renderer/gui/GuiElement.hpp>
#include <renderer/gui/controls/button/Button.hpp>
#include <renderer/gui/controls/label/Label.hpp>
#include <renderer/gui/controls/scroller/Scroller.hpp>
#include <renderer/gui/controls/text_field/TextField.hpp>

#include "ResourcePackTile.hpp"

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
		void ReloadTextures() override;

		void ScanResourcePacksFolder();

		void SetCurrentlySelectedResourcePack(const std::string& resourcePackName);
		std::string GetCurrentlySelectedResourcePack() const;

		// ----- Public Events -----
	  public:
		Event<const GuiElement*> RequestBackNavigation;
		Event<const std::string&> RequestResourcePackChange;

		// ----- Controls -----
	  private:
		Label m_Title_Label;
		Label m_Description_Label;

		TextField m_Search_TextField;
		Scroller m_ResourcePacks_Scroller;

		ResourcePackTile m_DefaultResourcePack_Tile;

		Button m_OpenPackFolder_Button;
		Button m_Done_Button;

		// ----- Internal Event Subscription and Handlers -----
	  private:
		std::vector<EventHandle> m_EventHandles;
		void SubscribeToControlEvents();

		void Handle_ResourcePackTileCheckedChanged(const ResourcePackTile& tile);
		void Handle_OpenPackFolder_Click(const Button& sender);
		void Handle_Done_Click(const Button& sender);

		// ----- Internal Members -----
	  private:
		Timer m_TimerScanResourcePacksFolder;

		mutable std::recursive_mutex m_MutexResourcePacks;
		std::vector<std::unique_ptr<ResourcePackTile>> m_ResourcePacksTiles;
		bool ContainsResourcePack(const std::string& resourcePackName) const;
		std::string m_CurrentlySelectedResourcePackName;

		ThreadSafeQueue<std::unique_ptr<ResourcePackTile>> m_ResourcePackTilesToDelete;
	};
} // namespace onion::voxel
