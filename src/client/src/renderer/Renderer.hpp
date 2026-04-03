#pragma once

#include <renderer/OpenGL.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <atomic>
#include <memory>
#include <stop_token>
#include <string>
#include <thread>

#include <onion/Timer.hpp>

#include <shared/physics/PhysicsEngine.hpp>
#include <shared/world/world_manager/WorldManager.hpp>

#include "entity_renderer/EntityRenderer.hpp"
#include "gui/Gui.hpp"
#include "inputs_manager/inputs_manager.hpp"
#include "key_binds/KeyBinds.hpp"
#include "world_renderer/WorldRenderer.hpp"

#include "EngineContext.hpp"

namespace onion::voxel
{
	struct ServerInfo
	{
		std::string Name;
		std::string Address;
		int Port = 0;
		int SimulationDistance = 0;
	};

	class Renderer
	{
	  public:
		enum class eRenderState
		{
			Menu,
			InGame
		};

		// ----- Constructor / Destructor -----
	  public:
		Renderer(std::shared_ptr<WorldManager> worldManager);
		~Renderer();

		// ----- Public API -----
	  public:
		void Start();
		void Stop();

		bool IsRunning() const noexcept;

		// ----- Getters / Setters -----
	  public:
		eRenderState GetRenderState() const;
		void SetRenderState(eRenderState renderState);

		std::shared_ptr<Player> GetPlayer() const;
		glm::vec3 GetPlayerPosition() const;

		void SetServerInfo(std::shared_ptr<ServerInfo> serverInfo);
		std::shared_ptr<ServerInfo> GetServerInfo() const;

		uint8_t GetRenderDistance() const;

		void SetPlayerUUID(const std::string& uuid);

		// ----- Events -----
	  public:
		Event<const WorldInfos&> RequestStartSingleplayerGame;
		Event<const Gui::MultiplayerGameStartInfo&> RequestStartMultiplayerGame;
		Event<uint8_t> EvtRenderDistanceChanged;
		Event<bool> RequestQuitToMainMenu;

		// ----- Internal Methods -----
	  private:
		void InitWindow();
		void InitOpenGl();
		void CleanupOpenGl();

		GLuint m_SceneFBO = 0;
		GLuint m_DepthRenderBuffer = 0;
		GLuint m_SceneColorTexture = 0;
		GLuint m_BlurFBO[2];
		GLuint m_BlurTexture[2];

		void RenderSceneToFBO();
		GLuint ApplyBlur(GLuint inputTexture);
		void PresentScene(GLuint texture);
		void RenderFullscreenQuad();

		Shader m_ScreenShader{AssetsManager::GetShadersDirectory() / "screen.vert",
							  AssetsManager::GetShadersDirectory() / "screen.frag"};
		Shader m_BlurShader{AssetsManager::GetShadersDirectory() / "blur.vert",
							AssetsManager::GetShadersDirectory() / "blur.frag"};

		// ----- Main Loop -----
	  private:
		std::atomic_bool m_IsRunning{false};
		void RenderThreadFunction(std::stop_token st);
		std::jthread m_ThreadRenderer;

		// ----- Render State -----
	  private:
		mutable std::mutex m_MutexRenderState;
		eRenderState m_RenderState = eRenderState::Menu;
		bool m_IsPaused = false;
		std::string m_PlayerUUID;

		// ----- GLFW -----
	  private:
		GLFWwindow* m_Window = nullptr;
		int m_WindowWidth = 800;
		int m_WindowHeight = 600;
		std::string m_WindowTitle = "Onion Voxel";
		std::filesystem::path m_WindowIconPath = AssetsManager::GetAppIconsDirectory() / "Vox_Client_Title.png";
		void SetupWindowIcon();

		double m_DeltaTime = 0.0f;
		double m_LastFrame = 0.0f;
		bool m_IsVsyncEnabled = true;
		uint32_t m_MaxFps = 60;
		void CapFPS(double targetFrameTime);

		// ----- Configurations -----
	  private:
		std::filesystem::path GetUserSettingsPath() const;
		void ApplyUserSettings(const UserSettingsChangedEventArgs& args);
		Timer m_TimerDelayedSaveUserSettings;
		void SaveUserSettings();

		// ----- Inputs -----
	  private:
		InputsManager m_InputsManager;
		std::vector<EventHandle> m_InputsManagerEventHandles;
		void SubscribeToInputsManagerEvents();

		void Handle_FramebufferResized(const FramebufferState& framebufferState);

		void RegisterInputs();
		void ProcessInputs();
		void ProcessGameplayInputs();

		KeyBinds m_KeyBinds;

		// ----- Actions -----
	  private:
		void PauseGame(bool pause);

		// ----- Resource Pack Manager -----
	  private:
		AssetsManager m_AssetsManager;

		// ------ World Manager ------
	  private:
		std::shared_ptr<WorldManager> m_WorldManager;

		void UpdatePlayerFromInputs();
		float m_PlayerFlySpeed = 5.0f;

		// ------ World Renderer ------
	  private:
		std::shared_ptr<Camera> m_Camera;
		Block m_HitBlock;
		float m_CameraSpeed = 5.0f;
		WorldRenderer m_WorldRenderer;
		bool m_IsFreeCamera = false;
		void UpdateCameraFromInputs();

		// ----- Entity Renderer -----
	  private:
		EntityRenderer m_EntityRenderer;

		// ----- Physics Engine -----
	  private:
		PhysicsEngine m_PhysicsEngine;

		// ----- Server Info -----
	  private:
		std::shared_ptr<ServerInfo> m_ServerInfo;

		// ------ GUI ------
	  private:
		Gui m_Gui;
		void SubscribeToGuiEvents();
		std::vector<EventHandle> m_EventHandles;

		void Handle_CursorStyleChangeRequest(const CursorStyle& style);
		void Handle_StartSingleplayerGameRequest(const WorldInfos& worldInfos);
		void Handle_StartMultiplayerGameRequest(const Gui::MultiplayerGameStartInfo& startInfo);
		void Handle_BackToGameRequest();
		void Handle_QuitToMainMenuRequest(bool quit);
		void Handle_ResourcePackChangeRequest(const std::string& resourcePackName);
		void Handle_UserSettingsChanged(const UserSettingsChangedEventArgs& args);

		// ----- ImGui -----
	  private:
		void InitImGui();
		void BeginImGuiFrame();
		void RenderDebugPanel();
		void RenderPhysicsDebugPanel();
		void EndImGuiFrame();
	};

}; // namespace onion::voxel
