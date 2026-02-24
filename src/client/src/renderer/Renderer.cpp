#include "Renderer.hpp"

#include <iostream>
#include <stdexcept>
#include <stop_token>
#include <thread>

namespace
{
	static void error_callback(int code, const char* desc)
	{
		std::fprintf(stderr, "GLFW error %d: %s\n", code, desc);
	}
} // namespace

namespace onion::voxel
{
	Renderer::Renderer() {}

	Renderer::~Renderer() {}

	void Renderer::Start()
	{
		std::cout << "Start Renderer" << std::endl;
		m_IsRunning.store(true);
		m_ThreadRenderer = std::jthread([this](std::stop_token st) { RenderThreadFunction(st); });
	}

	void Renderer::Stop()
	{
		std::cout << "Stop Renderer" << std::endl;

		if (m_ThreadRenderer.joinable())
		{
			m_ThreadRenderer.request_stop();
			m_ThreadRenderer.join();
		}
	}

	bool Renderer::IsRunning() const noexcept
	{
		return m_IsRunning.load();
	}

	void Renderer::InitWindow()
	{
		glfwSetErrorCallback(error_callback);

		if (!glfwInit())
		{
			std::cerr << "Failed to initialize GLFW" << std::endl;
			throw std::runtime_error("GLFW initialization failed");
		}

		// Request an OpenGL context
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		m_Window = glfwCreateWindow(m_WindowWidth, m_WindowHeight, m_WindowTitle.c_str(), nullptr, nullptr);
		if (!m_Window)
		{
			glfwTerminate();
			throw std::runtime_error("Failed to create GLFW window");
		}

		glfwMakeContextCurrent(m_Window);

		glfwSwapInterval(1); // vsync

		// Load OpenGL function pointers with glad
		if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
		{
			std::cerr << "Failed to initialize GLAD\n";
			throw std::runtime_error("GLAD initialization failed");
		}

		// Initialize Inputs Manager
		m_InputsManager.Init(m_Window);
		m_InputsManager.SetMouseCaptureEnabled(false);
		RegisterInputs();

		// Init ImGui
		InitImGui();
	}

	void Renderer::InitOpenGlState()
	{
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);

		//glEnable(GL_CULL_FACE);
		//glCullFace(GL_BACK);
		//glFrontFace(GL_CCW);

		glDisable(GL_CULL_FACE);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	void Renderer::RenderThreadFunction(std::stop_token st)
	{
		m_IsRunning.store(true);

		InitWindow();

		InitOpenGlState();

		Gui::Initialize();
		GuiElement::SetScreenSize(m_WindowWidth, m_WindowHeight);

		DemoPanel demoPanel("DemoPanel");
		demoPanel.Initialize();

		while (!st.stop_requested() && !glfwWindowShouldClose(m_Window))
		{
			// Start ImGui Frame
			BeginImGuiFrame();

			// Pool inputs
			m_InputsManager.PoolInputs();
			m_InputsSnapshot = m_InputsManager.GetInputsSnapshot();

			// Process Global Inputs
			ProcessInputs(m_InputsSnapshot);

			// Clear
			glClearColor(0.1f, 0.1f, 0.12f, 1.f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Calculate Delta Time
			double currentFrame = glfwGetTime();
			m_DeltaTime = currentFrame - m_LastFrame;
			m_LastFrame = currentFrame;

			demoPanel.Render();

			// Render Debug Panel
			RenderDebugPanel();

			//// Process Camera Movement
			//ProcessCameraMovement(m_InputsSnapshot);

			//// Get Camera projection, view and ProjView Matix
			//m_ProjectionMatrix = m_Camera.GetProjectionMatrix();
			//m_ViewMatrix = m_Camera.GetViewMatrix();
			//m_ViewProjMatrix = m_ProjectionMatrix * m_ViewMatrix;

			//// ------ TESTS MODELS ------
			//UpdateShaderModel();
			//DrawAppleModel();

			// End ImGui Frame
			EndImGuiFrame();

			glfwSwapBuffers(m_Window);
			glfwPollEvents();
		}

		demoPanel.Delete();

		// Cleanup
		CleanupOpenGl();
		Gui::Shutdown();

		m_IsRunning.store(false);
	}

	void Renderer::FramebufferSizeCallback(int width, int height)
	{
		glViewport(0, 0, width, height);

		m_WindowWidth = width;
		m_WindowHeight = height;

		GuiElement::SetScreenSize(m_WindowWidth, m_WindowHeight);
	}

	void Renderer::RegisterInputs()
	{
		m_InputIdUnfocus = m_InputsManager.RegisterInput(Key::Escape);
		m_InputIdFocus = m_InputsManager.RegisterInput(Key::Space);
	}

	void Renderer::ProcessInputs(const std::shared_ptr<InputsSnapshot>& inputs)
	{
		if (inputs->Framebuffer.Resized)
		{
			FramebufferSizeCallback(inputs->Framebuffer.Width, inputs->Framebuffer.Height);
		}

		if (inputs->GetKeyState(m_InputIdUnfocus).IsPressed && inputs->Mouse.CaptureEnabled)
		{
			m_InputsManager.SetMouseCaptureEnabled(false);
		}

		if (inputs->GetKeyState(m_InputIdFocus).IsPressed && !inputs->Mouse.CaptureEnabled)
		{
			m_InputsManager.SetMouseCaptureEnabled(true);
		}

		GuiElement::SetInputsSnapshot(inputs);
	}

	void Renderer::InitImGui()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

		ImGui::StyleColorsDark();

		ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
		ImGui_ImplOpenGL3_Init("#version 330");
	}

	void Renderer::BeginImGuiFrame()
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void Renderer::RenderDebugPanel()
	{
		ImGui::Begin("Debug Panel");

		ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

		static bool showWireframe = false;
		ImGui::Checkbox("Wireframe", &showWireframe);

		static float exposure = 1.0f;
		ImGui::SliderFloat("Exposure", &exposure, 0.1f, 5.0f);

		ImGui::End();
	}

	void Renderer::EndImGuiFrame()
	{
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	void Renderer::CleanupOpenGl() {}

} // namespace onion::voxel
