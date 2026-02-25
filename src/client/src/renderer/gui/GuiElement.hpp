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
		// ----- Constructor / Destructor -----
	  public:
		GuiElement(const std::string& name);
		virtual ~GuiElement();

		// ----- Public API -----
	  public:
		virtual void Render() = 0;
		virtual void Initialize() = 0;
		virtual void Delete() = 0;

		// ----- Getters / Setters -----
	  public:
		std::string GetName() const;
		void SetName(const std::string& name);

		// ----- Static Methods ----- (for shared resources and context)
	  public:
		static void SetScreenSize(int screenWidth, int screenHeight);
		static void SetInputsSnapshot(std::shared_ptr<InputsSnapshot> inputsSnapshot);

		// ----- Static Methods ----- (for Initialization and Unload)
	  public:
		static void Load();
		static void Unload();

		// ----- State Management -----
	  protected:
		void SetInitState(bool state);
		void SetDeletedState(bool state);

		// ----- Static Resources ----- (shared across all GUI elements)
	  protected:
		static Shader s_ShaderSprites;
		static Shader s_ShaderNineSliceSprites;
		static glm::mat4 s_ProjectionMatrix;
		static int s_ScreenWidth;
		static int s_ScreenHeight;

		static inline int s_GUI_SCALE = 4;

		static std::shared_ptr<InputsSnapshot> s_InputsSnapshot;

		// ----- Static Resources ----- (for text rendering)
	  protected:
		static Font s_TextFont;

		// ----- Instance Variables -----
	  private:
		std::string m_Name;

		std::atomic_bool m_HasBeenInit{false};
		std::atomic_bool m_HasBeenDeleted{false};
	};
} // namespace onion::voxel
