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

#include <shared/world/world_manager/WorldManager.hpp>

#include "gui/Gui.hpp"
#include "inputs_manager/inputs_manager.hpp"
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

		glm::vec3 GetPlayerPosition() const;

		void SetServerInfo(std::shared_ptr<ServerInfo> serverInfo);
		std::shared_ptr<ServerInfo> GetServerInfo() const;

		// ----- Events -----
	  public:
		Event<const std::filesystem::path&> RequestStartSingleplayerGame;
		Event<bool> RequestQuitToMainMenu;

		// ----- Internal Methods -----
	  private:
		void InitWindow();
		void InitOpenGlState();
		void CleanupOpenGl();

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

		// ----- GLFW -----
	  private:
		GLFWwindow* m_Window = nullptr;
		int m_WindowWidth = 800;
		int m_WindowHeight = 600;
		std::string m_WindowTitle = "Onion Voxel";
		std::filesystem::path m_WindowIconPath = GetAssetsPath() / "app_icons" / "Vox_Client_Title.png";
		void SetupWindowIcon();

		void FramebufferSizeCallback(int width, int height);

		double m_DeltaTime = 0.0f;
		double m_LastFrame = 0.0f;

		// ----- Inputs -----
	  private:
		InputsManager m_InputsManager;
		std::shared_ptr<InputsSnapshot> m_InputsSnapshot;

		void RegisterInputs();
		void ProcessInputs(const std::shared_ptr<InputsSnapshot>& inputs);
		void ProcessGameplayInputs(const std::shared_ptr<InputsSnapshot>& inputs);

		int m_InputIdMoveForward = -1;
		int m_InputIdMoveBackward = -1;
		int m_InputIdMoveLeft = -1;
		int m_InputIdMoveRight = -1;
		int m_InputIdMoveUp = -1;
		int m_InputIdMoveDown = -1;
		int m_InputIdSpeedUp = -1;
		int m_InputIdPause = -1;
		int m_InputIdUnfocus = -1;
		int m_InputIdFocus = -1;

		int m_InputIdPlaceBlock = -1;
		int m_InputIdRemoveBlock = -1;

		int m_InputIdGuiBack = -1;

		// ----- Actions -----
	  private:
		void PauseGame(bool pause);

		// ----- Resource Pack Manager -----
	  private:
		AssetsManager m_AssetsManager;

		// ------ World Manager ------
	  private:
		std::shared_ptr<WorldManager> m_WorldManager;

		// ------ World Renderer ------
	  private:
		std::shared_ptr<Camera> m_Camera;
		float m_CameraSpeed = 5.0f;
		WorldRenderer m_WorldRenderer;
		bool m_IsFreeCamera = true;
		void UpdateCameraFromInputs(const std::shared_ptr<InputsSnapshot>& inputs);

		// ----- Server Info -----
	  private:
		std::shared_ptr<ServerInfo> m_ServerInfo;

		// ------ GUI ------
	  private:
		Gui m_Gui;
		void SubscribeToGuiEvents();
		std::vector<EventHandle> m_EventHandles;

		void Handle_CursorStyleChangeRequest(const CursorStyle& style);
		void Handle_StartSingleplayerGameRequest(const std::filesystem::path& worldPath);
		void Handle_BackToGameRequest();
		void Handle_QuitToMainMenuRequest(bool quit);
		void Handle_ResourcePackChangeRequest(const std::string& resourcePackName);

		// ----- ImGui -----
	  private:
		void InitImGui();
		void BeginImGuiFrame();
		void RenderDebugPanel();
		void EndImGuiFrame();
	};

}; // namespace onion::voxel
