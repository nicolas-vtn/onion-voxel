#pragma once

#include <atomic>
#include <memory>
#include <string>

#include <glm/glm.hpp>

#include "../Variables.hpp"
#include "../inputs_manager/inputs_manager.hpp"
#include "../shader/shader.hpp"
#include "font/font.hpp"

namespace onion::voxel
{
	class GuiElement
	{
	  public:
		GuiElement(const std::string& name);
		virtual ~GuiElement();

		virtual void Render() = 0;
		virtual void Initialize() = 0;
		virtual void Delete() = 0;

		std::string GetName() const;
		void SetName(const std::string& name);

		static void SetScreenSize(int screenWidth, int screenHeight);
		static void SetInputsSnapshot(std::shared_ptr<InputsSnapshot> inputsSnapshot);

		static void Load();
		static void Unload();

	  protected:
		void SetInitState(bool state);
		void SetDeletedState(bool state);

	  protected:
		static Shader s_ShaderSprites;
		static Shader s_ShaderNineSliceSprites;
		static glm::mat4 s_ProjectionMatrix;
		static int s_ScreenWidth;
		static int s_ScreenHeight;

		static std::shared_ptr<InputsSnapshot> s_InputsSnapshot;

	  protected:
		static Font s_TextFont;

	  private:
		std::string m_Name;

		std::atomic_bool m_HasBeenInit{false};
		std::atomic_bool m_HasBeenDeleted{false};
	};
} // namespace onion::voxel
