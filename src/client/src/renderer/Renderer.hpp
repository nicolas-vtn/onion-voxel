#pragma once

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <atomic>
#include <memory>
#include <stop_token>
#include <string>
#include <thread>

#include "gui/Gui.hpp"
#include "inputs_manager/inputs_manager.hpp"
#include "world_renderer/WorldRenderer.hpp"

namespace onion::voxel
{
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
		Renderer();
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

		// ----- Actions -----
	  private:
		void PauseGame(bool pause);

		// ------ World Renderer ------
	  private:
		std::shared_ptr<Camera> m_Camera;
		float m_CameraSpeed = 5.0f;
		WorldRenderer m_WorldRenderer;
		bool m_IsFreeCamera = true;
		void UpdateCameraFromInputs(const std::shared_ptr<InputsSnapshot>& inputs);

		// ------ GUI ------
	  private:
		Gui m_Gui;
		void SubscribeToGuiEvents();
		std::vector<EventHandle> m_EventHandles;
		void Handle_CursorStyleChangeRequest(const CursorStyle& style);
		void Handle_StartSingleplayerGameRequest(const std::filesystem::path& worldPath);
		void Handle_GameResumeRequest(bool resume);
		void Handle_QuitToMainMenuRequest(bool quit);

		// ----- ImGui -----
	  private:
		void InitImGui();
		void BeginImGuiFrame();
		void RenderDebugPanel();
		void EndImGuiFrame();
	};

}; // namespace onion::voxel
