#pragma once

#include "layouts/demo_panel/DemoPanel.hpp"

#include <memory>
#include <mutex>

namespace onion::voxel
{
	class Gui
	{
	  public:
		enum class eMenu
		{
			None,
			DemoPanel,
			MainMenu,
			Singleplayer,
			Multiplayer,
			Settings,
			Gameplay,
		};

		// ----- Static Initialization / Shutdown -----
	  public:
		static void StaticInitialize();
		static void StaticShutdown();

		// ----- Constructor / Destructor -----
	  public:
		Gui();
		~Gui();

		// ----- Public API -----
	  public:
		void Initialize();
		void Render();
		void Shutdown();

		void SetActiveMenu(eMenu menu);
		eMenu GetActiveMenu() const;
		// ----- Panels -----
	  private:
		std::unique_ptr<DemoPanel> m_DemoPanel;

		// ----- Set Static States -----
	  public:
		static void SetInputsSnapshot(std::shared_ptr<InputsSnapshot> inputsSnapshot);
		static void SetScreenSize(int screenWidth, int screenHeight);

		// ----- States -----
	  private:
		mutable std::mutex m_MutexState;
		eMenu m_ActiveMenu = eMenu::None;
	};
} // namespace onion::voxel
