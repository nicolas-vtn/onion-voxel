#pragma once

#include <atomic>
#include <memory>
#include <string>

#include <glm/glm.hpp>

#include <onion/Event.hpp>

#include <renderer/EngineContext.hpp>
#include <renderer/Variables.hpp>
#include <renderer/gui/colored_background/ColoredBackground.hpp>
#include <renderer/gui/font/Font.hpp>
#include <renderer/inputs_manager/inputs_manager.hpp>
#include <renderer/shader/shader.hpp>

namespace onion::voxel
{
	class UserSettingsChangedEventArgs;

	enum class eMenu
	{
		None,
		DemoPanel,
		DemoScrollingPanel,
		MainMenu,
		Singleplayer,
		Multiplayer,
		Options,
		VideoSettings,
		Controls,
		MouseSettings,
		KeyBinds,
		Gameplay,
		Pause,
		MusicAndSounds,
		ResourcePacks,
	};

	std::string GetMenuName(eMenu menu);

	class GuiElement
	{
		// ----- Structs -----
	  public:
		struct Visibility
		{
			bool IsVisible = false;
			bool IsFullyVisible = false;
			glm::ivec2 VisibleAreaTopLeftCorner{0, 0};
			glm::ivec2 VisibleAreaBottomRightCorner{0, 0};
		};

		// ----- Constructor / Destructor -----
	  public:
		GuiElement(const std::string& name);
		virtual ~GuiElement();

		// ----- Public API -----
	  public:
		virtual void Render() = 0;
		virtual void Initialize() = 0;
		virtual void Delete() = 0;
		virtual void ReloadTextures() = 0;

		// ----- Getters / Setters -----
	  public:
		std::string GetName() const;
		void SetName(const std::string& name);

		Visibility GetVisibility() const;
		virtual void SetVisibility(const Visibility& visibility);

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

		// ----- Private Members -----
	  private:
		Visibility m_Visibility{true, true};

		// ----- Static Resources ----- (shared across all GUI elements)
	  protected:
		static Shader s_ShaderSprites;
		static Shader s_ShaderNineSliceSprites;
		static glm::mat4 s_ProjectionMatrix;
		static int s_ScreenWidth;
		static int s_ScreenHeight;

		static inline glm::vec3 s_ColorMainText{1.f, 1.f, 1.f};
		static inline glm::vec3 s_ColorSecondaryText{0.5f, 0.5f, 0.5f};
		static inline glm::vec3 s_ColorTertiaryText{0.3333f, 0.3333f, 0.3333f};

		static inline std::atomic_int s_GuiScale = 4;

		static std::shared_ptr<InputsSnapshot> s_InputsSnapshot;

		// ----- Static Resources ----- (for text rendering)
	  protected:
		static Font s_TextFont;

		static inline const std::filesystem::path s_BasePathGuiAssets =
			std::filesystem::path("assets") / "minecraft" / "textures" / "gui";

		static inline const std::filesystem::path s_PathFont =
			std::filesystem::path("assets") / "minecraft" / "textures" / "font" / "ascii.png";

		// ----- Instance Variables -----
	  private:
		std::string m_Name;

		std::atomic_bool m_HasBeenInit{false};
		std::atomic_bool m_HasBeenDeleted{false};
	};

	// ----- Event Args Classes -----
	class UserSettingsChangedEventArgs
	{

	  public:
		UserSettingsChangedEventArgs(const UserSettings& newSettings, bool allChanged = false)
			: NewSettings(newSettings)
		{
			if (allChanged)
			{
				RenderDistance_Changed = true;
				SimulationDistance_Changed = true;
				MaxFPS_Changed = true;
				VSyncEnabled_Changed = true;
				FOV_Changed = true;
				MouseSensitivity_Changed = true;
				MouseScrollSensitivity_Changed = true;
				KeyBinds_Changed = true;
			}
		}

	  public:
		UserSettings NewSettings;

		bool RenderDistance_Changed = false;
		bool SimulationDistance_Changed = false;

		bool MaxFPS_Changed = false;
		bool VSyncEnabled_Changed = false;

		bool FOV_Changed = false;

		bool MouseSensitivity_Changed = false;
		bool MouseScrollSensitivity_Changed = false;

		bool KeyBinds_Changed = false;
		std::unordered_map<eAction, Key> ChangedKeyBinds; // Action -> NewKey
	};
} // namespace onion::voxel
