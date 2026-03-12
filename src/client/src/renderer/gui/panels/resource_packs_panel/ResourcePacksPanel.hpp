#pragma once

#include <mutex>
#include <string>
#include <vector>

#include <onion/ThreadSafeQueue.hpp>
#include <onion/Timer.hpp>

#include "../../GuiElement.hpp"
#include "../../controls/button/Button.hpp"
#include "../../controls/label/Label.hpp"
#include "../../controls/sprite/Sprite.hpp"

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

		void ScanResourcePacksFolder();

		// ----- Public Events -----
	  public:
		Event<const GuiElement*> RequestBackNavigation;

		// ----- Internal Structs -----
	  private:
		struct ResourcePackInfo
		{
			std::string Name;
			std::string Description;
			std::unique_ptr<Sprite> Thumbnail;
		};

		// ----- Controls -----
	  private:
		Label m_Title_Label;
		Label m_Description_Label;

		Button m_OpenPackFolder_Button;
		Button m_Done_Button;

		// ----- Internal Event Subscription and Handlers -----
	  private:
		std::vector<EventHandle> m_EventHandles;
		void SubscribeToControlEvents();

		void Handle_OpenPackFolder_Click(const Button& sender);
		void Handle_Done_Click(const Button& sender);

		// ----- Internal Members -----
	  private:
		Timer m_TimerScanResourcePacksFolder;

		mutable std::mutex m_MutexResourcePacks;
		std::vector<ResourcePackInfo> m_ResourcePacks;
		bool ContainsResourcePack(const std::string& resourcePackName) const;

		ThreadSafeQueue<ResourcePackInfo> m_ResourcePacksInfosToDelete;
	};
} // namespace onion::voxel
