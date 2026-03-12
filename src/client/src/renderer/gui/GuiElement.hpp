#pragma once

#include <atomic>
#include <memory>
#include <string>

#include <glm/glm.hpp>

#include <onion/Event.hpp>

#include "../Variables.hpp"
#include "../inputs_manager/inputs_manager.hpp"
#include "../shader/shader.hpp"
#include "font/font.hpp"

namespace onion::voxel
{
	enum class eMenu
	{
		None,
		DemoPanel,
		MainMenu,
		Singleplayer,
		Multiplayer,
		Options,
		Gameplay,
		Pause,
		MusicAndSounds,
		ResourcePacks,
		Controls,
	};

	std::string GetMenuName(eMenu menu);

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

		static void SetGuiScale(int scale);
		static int GetGuiScale();

		// ----- Static Methods ----- (for shared resources and context)
	  public:
		static void SetScreenSize(int screenWidth, int screenHeight);
		static void SetInputsSnapshot(std::shared_ptr<InputsSnapshot> inputsSnapshot);
		static inline std::atomic_bool s_IsBackPressed;

		// ----- Static Methods ----- (for Initialization and Unload)
	  public:
		static void Load();
		static void Unload();

		// ----- Static Events ----- (for shared resources)
	  public:
		static Event<const CursorStyle&> RequestCursorStyleChange;

		// ----- State Management -----
	  protected:
		void SetInitState(bool state);
		bool HasBeenInit() const;
		void SetDeletedState(bool state);
		bool HasBeenDeleted() const;

		// ----- Static Resources ----- (shared across all GUI elements)
	  protected:
		static Shader s_ShaderSprites;
		static Shader s_ShaderNineSliceSprites;
		static glm::mat4 s_ProjectionMatrix;
		static int s_ScreenWidth;
		static int s_ScreenHeight;

		static inline glm::vec3 s_ColorMainText{1.f, 1.f, 1.f};
		static inline glm::vec3 s_ColorSecondaryText{0.5f, 0.5f, 0.5f};
		static inline glm::vec3 s_ColorTertiaryText{0.333f, 0.333f, 0.333f};

		static inline std::atomic_int s_GuiScale = 4;

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
