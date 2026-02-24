#pragma once

#include "../../GuiElement.hpp"
#include "../../controls/button/Button.hpp"
#include "../../controls/sprite/Sprite.hpp"

namespace onion::voxel
{
	class Button;

	class DemoPanel : public GuiElement
	{
	  public:
		DemoPanel(const std::string& name);
		~DemoPanel() override = default;

		void Render() override;
		void Initialize() override;
		void Delete() override;

	  private:
		Button m_Button;
		Button m_Button2;

		std::string m_SpritePath = (GetAssetsPath() / "textures" / "OnionVoxelTitle.png").string();
		Sprite m_Sprite;
	};
} // namespace onion::voxel
