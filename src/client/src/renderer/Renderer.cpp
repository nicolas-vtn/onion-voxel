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
	Renderer::Renderer(std::shared_ptr<WorldManager> worldManager)
		: m_WorldManager(worldManager), m_Camera(std::make_shared<Camera>(glm::vec3(0.0f, 0.0f, 0.0f), 800, 600)),
		  m_WorldRenderer(worldManager, m_Camera)
	{
	}

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

	Renderer::eRenderState Renderer::GetRenderState() const
	{
		std::lock_guard lock(m_MutexRenderState);
		return m_RenderState;
	}

	void Renderer::SetRenderState(eRenderState renderState)
	{
		std::lock_guard lock(m_MutexRenderState);
		m_RenderState = renderState;

		if (m_RenderState == eRenderState::InGame)
		{
			m_Gui.SetActiveMenu(eMenu::Gameplay);
		}

		if (m_RenderState == eRenderState::Menu)
		{
			m_Gui.SetActiveMenu(eMenu::MainMenu);
		}
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

		// Set up window icon
		SetupWindowIcon();

		// Initialize Inputs Manager
		m_InputsManager.Init(m_Window);
		m_InputsManager.SetMouseCaptureEnabled(false);
		m_InputsManager.SetCursorStyle(CursorStyle::Arrow);
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

		Gui::StaticInitialize();
		Gui::SetScreenSize(m_WindowWidth, m_WindowHeight);

		m_Gui.Initialize();
		SubscribeToGuiEvents();
		m_Gui.SetGameVersion("0.1.0");

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

			// Render World
			if (GetRenderState() == eRenderState::InGame)
			{
				m_WorldRenderer.Render();
			}

			// Render GUI
			m_Gui.Render();

			// Render Debug Panel
			RenderDebugPanel();

			// End ImGui Frame
			EndImGuiFrame();

			// Swap buffers and poll events
			glfwSwapBuffers(m_Window);
			glfwPollEvents();
		}

		// Cleanup
		CleanupOpenGl();

		m_Gui.Shutdown();
		m_InputsManager.Delete();

		Gui::StaticShutdown();

		m_IsRunning.store(false);
	}

	void Renderer::SetupWindowIcon()
	{
		if (!std::filesystem::exists(m_WindowIconPath))
		{
			std::cerr << "Window icon not found at path: " << m_WindowIconPath << std::endl;
			return;
		}

		// Load the icon image using stb_image
		int width, height, channels;

		unsigned char* pixels = stbi_load(m_WindowIconPath.string().c_str(), &width, &height, &channels, 4);

		if (!pixels)
		{
			std::cerr << "Failed to load icon\n";
		}

		// Set up the GLFWimage structure
		GLFWimage image;
		image.width = width;
		image.height = height;
		image.pixels = pixels;

		// Set the window icon
		glfwSetWindowIcon(m_Window, 1, &image);

		// Free the loaded image data
		stbi_image_free(pixels);
	}

	void Renderer::FramebufferSizeCallback(int width, int height)
	{
		glViewport(0, 0, width, height);

		m_WindowWidth = width;
		m_WindowHeight = height;

		GuiElement::SetScreenSize(m_WindowWidth, m_WindowHeight);

		if (m_WindowWidth != 0 && m_WindowHeight != 0)
		{
			m_Camera->SetAspectRatio(static_cast<float>(m_WindowWidth) / static_cast<float>(m_WindowHeight));
		}
	}

	void Renderer::RegisterInputs()
	{
		m_InputIdMoveForward = m_InputsManager.RegisterInput(Key::W);
		m_InputIdMoveBackward = m_InputsManager.RegisterInput(Key::S);
		m_InputIdMoveLeft = m_InputsManager.RegisterInput(Key::A);
		m_InputIdMoveRight = m_InputsManager.RegisterInput(Key::D);
		m_InputIdMoveUp = m_InputsManager.RegisterInput(Key::Space);
		m_InputIdMoveDown = m_InputsManager.RegisterInput(Key::LeftShift);
		m_InputIdSpeedUp = m_InputsManager.RegisterInput(Key::LeftControl);

		m_InputIdUnfocus = m_InputsManager.RegisterInput(Key::KP8);
		m_InputIdFocus = m_InputsManager.RegisterInput(Key::KP7);
		m_InputIdPause = m_InputsManager.RegisterInput(Key::Escape);
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

		if (inputs->GetKeyState(m_InputIdPause).IsPressed && GetRenderState() == eRenderState::InGame)
		{
			PauseGame(true);
		}

		GuiElement::SetInputsSnapshot(inputs);

		// Process Camera Movement Inputs (Only if in Free Camera Mode)
		UpdateCameraFromInputs(inputs);
	}

	void Renderer::PauseGame(bool pause)
	{
		if (pause && !m_IsPaused)
		{
			m_Gui.SetActiveMenu(eMenu::Pause);

			// Disable mouse capture when paused
			m_InputsManager.SetMouseCaptureEnabled(false);
		}
		else if (!pause && m_IsPaused)
		{
			m_Gui.SetActiveMenu(eMenu::Gameplay);

			// Enable mouse capture when resuming
			m_InputsManager.SetMouseCaptureEnabled(true);
		}

		m_IsPaused = pause;
	}

	void Renderer::UpdateCameraFromInputs(const std::shared_ptr<InputsSnapshot>& inputs)
	{
		if (!m_IsFreeCamera)
			return;

		if (!inputs->Mouse.CaptureEnabled)
		{
			return; // If mouse capture is not enabled, skip camera movement processing
		}

		// Camera's Orientation
		if (inputs->Mouse.MovementOffsetChanged)
		{
			const double sensitivity = 0.1f;
			const double xoffset = inputs->Mouse.Xoffset * sensitivity;
			const double yoffset = inputs->Mouse.Yoffset * sensitivity;

			m_Camera->SetYaw(m_Camera->GetYaw() + xoffset);
			m_Camera->SetPitch(m_Camera->GetPitch() + yoffset);

			if (m_Camera->GetPitch() > 89.0f)
				m_Camera->SetPitch(89.0f);
			if (m_Camera->GetPitch() < -89.0f)
				m_Camera->SetPitch(-89.0f);
		}

		// Adjust camera Speed
		if (inputs->Mouse.ScrollOffsetChanged)
		{
			const double xoffset = inputs->Mouse.ScrollXoffset;
			const double yoffset = inputs->Mouse.ScrollYoffset;
			if (yoffset != 0.f)
			{
				float coeefIncrease = 1.3f;
				float coeefDecrease = 0.7f;
				if (yoffset > 0)
				{
					m_CameraSpeed *= coeefIncrease; // Speed up
				}
				else if (yoffset < 0)
				{
					m_CameraSpeed *= coeefDecrease; // Slow down
				}
			}
		}

		// Camera's Position
		float velocity = m_CameraSpeed * m_DeltaTime;

		if (inputs->GetKeyState(m_InputIdSpeedUp).IsPressed)
		{
			velocity *= 2.0f; // Double speed if left control is pressed
		}

		// Flatten the front vector for XZ movement
		glm::vec3 CamFront = m_Camera->GetFront();
		glm::vec3 frontXZ = glm::normalize(glm::vec3(CamFront.x, 0.0f, CamFront.z));
		const glm::vec3 Up(0.0f, 1.0f, 0.0f); // Up vector

		if (inputs->GetKeyState(m_InputIdMoveForward).IsPressed)
			m_Camera->SetPosition(m_Camera->GetPosition() + frontXZ * velocity);
		if (inputs->GetKeyState(m_InputIdMoveBackward).IsPressed)
			m_Camera->SetPosition(m_Camera->GetPosition() - frontXZ * velocity);
		if (inputs->GetKeyState(m_InputIdMoveLeft).IsPressed)
			m_Camera->SetPosition(m_Camera->GetPosition() - glm::normalize(glm::cross(frontXZ, Up)) * velocity);
		if (inputs->GetKeyState(m_InputIdMoveRight).IsPressed)
			m_Camera->SetPosition(m_Camera->GetPosition() + glm::normalize(glm::cross(frontXZ, Up)) * velocity);
		if (inputs->GetKeyState(m_InputIdMoveUp).IsPressed) // Jump / up
			m_Camera->SetPosition(m_Camera->GetPosition() + Up * velocity);
		if (inputs->GetKeyState(m_InputIdMoveDown).IsPressed) // Down
			m_Camera->SetPosition(m_Camera->GetPosition() - Up * velocity);
	}

	void Renderer::SubscribeToGuiEvents()
	{
		m_EventHandles.push_back(m_Gui.RequestCursorStyleChange.Subscribe([this](const CursorStyle& style)
																		  { Handle_CursorStyleChangeRequest(style); }));

		m_EventHandles.push_back(m_Gui.RequestStartSingleplayerGame.Subscribe(
			[this](const std::filesystem::path& worldPath) { Handle_StartSingleplayerGameRequest(worldPath); }));

		m_EventHandles.push_back(
			m_Gui.RequestGameResume.Subscribe([this](bool resume) { Handle_GameResumeRequest(resume); }));

		m_EventHandles.push_back(
			m_Gui.RequestQuitToMainMenu.Subscribe([this](bool quit) { Handle_QuitToMainMenuRequest(quit); }));
	}

	void Renderer::Handle_CursorStyleChangeRequest(const CursorStyle& style)
	{
		m_InputsManager.SetCursorStyle(style);
	}

	void Renderer::Handle_StartSingleplayerGameRequest(const std::filesystem::path& worldPath)
	{
		RequestStartSingleplayerGame.Trigger(worldPath);

		// Enable mouse capture for gameplay
		m_InputsManager.SetMouseCaptureEnabled(true);

		m_IsPaused = false;
	}

	void Renderer::Handle_GameResumeRequest(bool resume)
	{
		PauseGame(!resume);
	}

	void Renderer::Handle_QuitToMainMenuRequest(bool quit)
	{
		if (GetRenderState() == eRenderState::InGame)
		{
			// Stops the Client Game
			RequestQuitToMainMenu.Trigger(quit);

			m_WorldManager->RemoveAllChunks();
			m_WorldRenderer.DeleteChunkMeshes();

			m_IsPaused = false;
		}
	}

	void Renderer::InitImGui()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

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

		// ----- Camera Debug -----
		if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
		{
			glm::vec3 position = m_Camera->GetPosition();
			glm::vec3 front = m_Camera->GetFront();
			float yaw = m_Camera->GetYaw();
			float pitch = m_Camera->GetPitch();
			float fov = m_Camera->GetFovY();
			float aspect = m_Camera->GetAspectRatio();

			if (ImGui::DragFloat3("Position", &position.x, 0.1f))
				m_Camera->SetPosition(position);

			if (ImGui::DragFloat3("Front", &front.x, 0.01f))
				m_Camera->SetFront(front);

			if (ImGui::DragFloat("Yaw", &yaw, 0.5f))
				m_Camera->SetYaw(yaw);

			if (ImGui::DragFloat("Pitch", &pitch, 0.5f, -89.f, 89.f))
				m_Camera->SetPitch(pitch);

			if (ImGui::DragFloat("FOV", &fov, 0.1f, 1.f, 120.f))
				m_Camera->SetFovY(fov);

			if (ImGui::DragFloat("Aspect Ratio", &aspect, 0.01f))
				m_Camera->SetAspectRatio(aspect);

			ImGui::DragFloat("Speed", &m_CameraSpeed, 0.1f, 0.1f, 50.f);
		}

		ImGui::End();
	}

	void Renderer::EndImGuiFrame()
	{
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	void Renderer::CleanupOpenGl() {}

} // namespace onion::voxel
