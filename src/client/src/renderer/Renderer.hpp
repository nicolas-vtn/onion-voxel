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

namespace onion::voxel
{
	class Renderer
	{
	  public:
		Renderer();
		~Renderer();

		void Start();
		void Stop();

		bool IsRunning() const noexcept;

	  private:
		void InitWindow();
		void InitOpenGlState();
		void CleanupOpenGl();

		std::atomic_bool m_IsRunning{false};
		void RenderThreadFunction(std::stop_token st);
		std::jthread m_ThreadRenderer;

		//GLFW
	  private:
		GLFWwindow* m_Window = nullptr;
		int m_WindowWidth = 800;
		int m_WindowHeight = 600;
		std::string m_WindowTitle = "Onion Voxel";

		void FramebufferSizeCallback(int width, int height);

	  private:
		double m_DeltaTime = 0.0f;
		double m_LastFrame = 0.0f;

	  private:
		InputsManager m_InputsManager;
		std::shared_ptr<InputsSnapshot> m_InputsSnapshot;

		void RegisterInputs();
		void ProcessInputs(const std::shared_ptr<InputsSnapshot>& inputs);

		//int m_InputIdMoveForward = -1;
		//int m_InputIdMoveBackward = -1;
		//int m_InputIdMoveLeft = -1;
		//int m_InputIdMoveRight = -1;
		//int m_InputIdMoveUp = -1;
		//int m_InputIdMoveDown = -1;
		//int m_InputIdSpeedUp = -1;
		int m_InputIdUnfocus = -1;
		int m_InputIdFocus = -1;

		// ----- ImGui -----
	  private:
		void InitImGui();
		void BeginImGuiFrame();
		void RenderDebugPanel();
		void EndImGuiFrame();
	};

}; // namespace onion::voxel
