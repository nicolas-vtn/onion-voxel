#pragma once

#include "layouts/demo_panel/DemoPanel.hpp"

namespace onion::voxel
{
	class Gui
	{
	  public:
		static void Initialize() { GuiElement::Load(); }
		static void Shutdown() { GuiElement::Unload(); }
	};
} // namespace onion::voxel
