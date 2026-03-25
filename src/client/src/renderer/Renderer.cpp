#include "Renderer.hpp"

#include <iostream>
#include <stdexcept>
#include <stop_token>
#include <thread>

#include <shared/utils/Utils.hpp>

#include "debug_draws/DebugDraws.hpp"

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
		: m_WorldManager(worldManager), m_Camera(std::make_shared<Camera>(glm::vec3(1.0f, 120.0f, 1.0f), 800, 600)),
		  m_WorldRenderer(worldManager, m_Camera), m_KeyBinds(m_InputsManager)
	{
		// Sets the Engine Context
		EngineContext::Initialize(worldManager.get(), &m_AssetsManager, &m_InputsManager);
	}

	Renderer::~Renderer()
	{
		m_EventHandles.clear();
	}

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

	glm::vec3 Renderer::GetPlayerPosition() const
	{
		return m_Camera->GetPosition();
	}

	void Renderer::SetServerInfo(std::shared_ptr<ServerInfo> serverInfo)
	{
		m_ServerInfo = serverInfo;
	}

	std::shared_ptr<ServerInfo> Renderer::GetServerInfo() const
	{
		return m_ServerInfo;
	}

	void Renderer::SetPlayerUUID(const std::string& uuid)
	{
		m_PlayerUUID = uuid;
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
		SubscribeToInputsManagerEvents();
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
		DebugDraws::ScreenWidth = m_WindowWidth;
		DebugDraws::ScreenHeight = m_WindowHeight;

		m_Gui.Initialize();
		m_WorldRenderer.Initialize();
		SubscribeToGuiEvents();

		while (!st.stop_requested() && !glfwWindowShouldClose(m_Window))
		{
			// Clear
			glClearColor(0.1f, 0.1f, 0.12f, 1.f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Start ImGui Frame
			BeginImGuiFrame();

			// Pool inputs
			m_InputsManager.PoolInputs();
			//m_InputsSnapshot = m_InputsManager.GetInputsSnapshot();

			// Process Global Inputs
			ProcessInputs();

			// Calculate Delta Time
			double currentFrame = glfwGetTime();
			m_DeltaTime = currentFrame - m_LastFrame;
			m_LastFrame = currentFrame;

			const glm::mat4 viewMatrix = m_Camera->GetViewMatrix();
			const glm::mat4 projectionMatrix = m_Camera->GetProjectionMatrix();
			const glm::mat4 viewProjectionMatrix = m_Camera->GetViewProjectionMatrix();

			// DEBUG : Sets DebugDraws ViewProj Matrix
			DebugDraws::SetViewProjMatrix(viewProjectionMatrix);

			// Render World
			if (GetRenderState() == eRenderState::InGame)
			{
				m_WorldRenderer.Render();
				const auto& entityMana = m_WorldManager->Entities;
				if (entityMana->IsPlayerExists(m_PlayerUUID))
				{
					m_WorldManager->SetPlayerPosition(m_PlayerUUID, m_Camera->GetPosition());
				}
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
		m_WorldRenderer.Unload();

		Gui::StaticShutdown();
		WorldRenderer::StaticUnload();
		DebugDraws::StaticUnload();

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

	void Renderer::SubscribeToInputsManagerEvents()
	{
		m_InputsManagerEventHandles.push_back(m_InputsManager.EventFramebufferResized.Subscribe(
			[this](const FramebufferState& framebufferState) { Handle_FramebufferResized(framebufferState); }));
	}

	void Renderer::Handle_FramebufferResized(const FramebufferState& framebufferState)
	{
		glViewport(0, 0, framebufferState.Width, framebufferState.Height);

		m_WindowWidth = framebufferState.Width;
		m_WindowHeight = framebufferState.Height;

		DebugDraws::ScreenWidth = m_WindowWidth;
		DebugDraws::ScreenHeight = m_WindowHeight;

		if (m_WindowWidth != 0 && m_WindowHeight != 0)
		{
			m_Camera->SetAspectRatio(static_cast<float>(m_WindowWidth) / static_cast<float>(m_WindowHeight));
		}
	}

	void Renderer::RegisterInputs()
	{
		// Eventually Loads from Config File or something like that, but for now it's hardcoded

		InputConfig everyFrame(false, 0, 0, 0);
		InputConfig noRepeat(true, 9999999, 0, 0);
		InputConfig repeatWithDelay(true, 0.6f, 0.4f, 0.5f);

		m_KeyBinds.RemapAction(eAction::MoveForward, Key::W, everyFrame);
		m_KeyBinds.RemapAction(eAction::MoveBackward, Key::S, everyFrame);
		m_KeyBinds.RemapAction(eAction::MoveLeft, Key::A, everyFrame);
		m_KeyBinds.RemapAction(eAction::MoveRight, Key::D, everyFrame);
		m_KeyBinds.RemapAction(eAction::Jump, Key::Space, everyFrame);
		m_KeyBinds.RemapAction(eAction::Crouch, Key::LeftShift, everyFrame);
		m_KeyBinds.RemapAction(eAction::Sprint, Key::LeftControl, everyFrame);

		m_KeyBinds.RemapAction(eAction::ToggleMouseCapture, Key::KP7, noRepeat);
		m_KeyBinds.RemapAction(eAction::Pause, Key::Escape, noRepeat);
		m_KeyBinds.RemapAction(eAction::CloseMenu, Key::Escape, noRepeat);

		m_KeyBinds.RemapAction(eAction::Attack, Key::MouseButtonLeft, repeatWithDelay);
		m_KeyBinds.RemapAction(eAction::Interact, Key::MouseButtonRight, repeatWithDelay);
	}

	void Renderer::ProcessInputs()
	{
		KeyState toggleMouseCaptureKeyState = m_KeyBinds.GetKeyState(eAction::ToggleMouseCapture);
		if (toggleMouseCaptureKeyState.IsPressed)
		{
			bool mouseCapture = m_InputsManager.IsMouseCaptureEnabled();
			m_InputsManager.SetMouseCaptureEnabled(!mouseCapture);
		}

		KeyState pauseKeyState = m_KeyBinds.GetKeyState(eAction::Pause);
		if (pauseKeyState.IsPressed && GetRenderState() == eRenderState::InGame)
		{
			PauseGame(true);
		}

		auto inputs = m_InputsManager.GetInputsSnapshot();
		GuiElement::SetInputsSnapshot(inputs);
		KeyState closeMenuKeyState = m_KeyBinds.GetKeyState(eAction::CloseMenu);
		GuiElement::s_IsBackPressed = closeMenuKeyState.IsPressed;

		// Process Camera Movement Inputs (Only if in Free Camera Mode)
		if (m_IsFreeCamera)
			UpdateCameraFromInputs();

		if (m_RenderState == eRenderState::InGame && !m_IsPaused)
			ProcessGameplayInputs();
	}

	void Renderer::ProcessGameplayInputs()
	{
		// Raycast from the camera's position.
		glm::vec3 rayOrigin = m_Camera->GetPosition();
		glm::vec3 rayDirection = m_Camera->GetFront();

		Block hitBlock = Block(glm::ivec3(), BlockId::Air);
		Block prevBlock = Block(glm::ivec3(), BlockId::Air);
		for (int i = 0; i < 100; i++)
		{
			float delta = 0.1f; // Step size for ray marching
			glm::vec3 checkPos = rayOrigin + rayDirection * delta * static_cast<float>(i);
			hitBlock.Position = glm::floor(checkPos);
			hitBlock.State = m_WorldManager->GetBlock(hitBlock.Position);
			if (hitBlock.ID() != BlockId::Air)
			{
				break;
			}
			prevBlock = hitBlock;
		}

		if (hitBlock.ID() != BlockId::Air)
		{
			DebugDraws::DrawBlockOutline(hitBlock.Position, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), 3, true);
			//DebugDraws::DrawBlockOutline(prevBlockPos, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), 3, true);
		}

		KeyState attackKeyState = m_KeyBinds.GetKeyState(eAction::Attack);
		if (attackKeyState.IsPressed)
		{
			Block airBlockToPlace = Block(hitBlock.Position, BlockId::Air);

			bool success = m_WorldManager->SetBlock(
				airBlockToPlace, WorldManager::BlocksChangedEventArgs::eOrigin::PlayerAction, true);

			std::cout << "Attempting to remove block at " << hitBlock.Position.x << ", " << hitBlock.Position.y << ", "
					  << hitBlock.Position.z << " - Success: " << (success ? "Yes" : "No") << std::endl;
		}

		KeyState interactKeyState = m_KeyBinds.GetKeyState(eAction::Interact);
		if (interactKeyState.IsPressed)
		{
			Block blockToPlace = Block(prevBlock.Position, BlockId::Cobblestone);

			bool success = m_WorldManager->SetBlock(
				blockToPlace, WorldManager::BlocksChangedEventArgs::eOrigin::PlayerAction, true);

			std::cout << "Attempting to place block at " << prevBlock.Position.x << ", " << prevBlock.Position.y << ", "
					  << prevBlock.Position.z << " - Success: " << (success ? "Yes" : "No") << std::endl;
		}
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

	void Renderer::UpdateCameraFromInputs()
	{
		if (!m_InputsManager.IsMouseCaptureEnabled())
		{
			return; // If mouse capture is not enabled, skip camera movement processing
		}

		auto inputs = m_InputsManager.GetInputsSnapshot();

		// Camera's Orientation
		if (inputs->Mouse.MovementOffsetChanged)
		{
			const double sensitivity = 0.1f;
			const double xoffset = inputs->Mouse.Xoffset * sensitivity;
			const double yoffset = inputs->Mouse.Yoffset * sensitivity;

			m_Camera->SetYaw(m_Camera->GetYaw() + (float) xoffset);
			m_Camera->SetPitch(m_Camera->GetPitch() + (float) yoffset);

			if (m_Camera->GetPitch() > 89.0f)
				m_Camera->SetPitch(89.0f);
			if (m_Camera->GetPitch() < -89.0f)
				m_Camera->SetPitch(-89.0f);
		}

		// Adjust camera Speed
		if (inputs->Mouse.ScrollOffsetChanged)
		{
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
		float velocity = m_CameraSpeed * (float) m_DeltaTime;

		KeyState speedUpKeyState = m_KeyBinds.GetKeyState(eAction::Sprint);
		if (speedUpKeyState.IsPressed)
		{
			velocity *= 2.0f; // Double speed if left control is pressed
		}

		// Flatten the front vector for XZ movement
		glm::vec3 CamFront = m_Camera->GetFront();
		glm::vec3 frontXZ = glm::normalize(glm::vec3(CamFront.x, 0.0f, CamFront.z));
		const glm::vec3 Up(0.0f, 1.0f, 0.0f); // Up vector

		KeyState moveForwardKeyState = m_KeyBinds.GetKeyState(eAction::MoveForward);
		KeyState moveBackwardKeyState = m_KeyBinds.GetKeyState(eAction::MoveBackward);
		KeyState moveLeftKeyState = m_KeyBinds.GetKeyState(eAction::MoveLeft);
		KeyState moveRightKeyState = m_KeyBinds.GetKeyState(eAction::MoveRight);
		KeyState moveUpKeyState = m_KeyBinds.GetKeyState(eAction::Jump);
		KeyState moveDownKeyState = m_KeyBinds.GetKeyState(eAction::Crouch);

		if (moveForwardKeyState.IsPressed)
			m_Camera->SetPosition(m_Camera->GetPosition() + frontXZ * velocity);
		if (moveBackwardKeyState.IsPressed)
			m_Camera->SetPosition(m_Camera->GetPosition() - frontXZ * velocity);
		if (moveLeftKeyState.IsPressed)
			m_Camera->SetPosition(m_Camera->GetPosition() - glm::normalize(glm::cross(frontXZ, Up)) * velocity);
		if (moveRightKeyState.IsPressed)
			m_Camera->SetPosition(m_Camera->GetPosition() + glm::normalize(glm::cross(frontXZ, Up)) * velocity);
		if (moveUpKeyState.IsPressed)
			m_Camera->SetPosition(m_Camera->GetPosition() + Up * velocity);
		if (moveDownKeyState.IsPressed)
			m_Camera->SetPosition(m_Camera->GetPosition() - Up * velocity);
	}

	void Renderer::SubscribeToGuiEvents()
	{
		m_EventHandles.push_back(m_Gui.RequestCursorStyleChange.Subscribe([this](const CursorStyle& style)
																		  { Handle_CursorStyleChangeRequest(style); }));

		m_EventHandles.push_back(m_Gui.RequestStartSingleplayerGame.Subscribe(
			[this](const std::filesystem::path& worldPath) { Handle_StartSingleplayerGameRequest(worldPath); }));

		m_EventHandles.push_back(m_Gui.RequestStartMultiplayerGame.Subscribe(
			[this](const Gui::MultiplayerGameStartInfo& startInfo) { Handle_StartMultiplayerGameRequest(startInfo); }));

		m_EventHandles.push_back(m_Gui.RequestBackToGame.Subscribe([this](bool) { Handle_BackToGameRequest(); }));

		m_EventHandles.push_back(
			m_Gui.RequestQuitToMainMenu.Subscribe([this](bool quit) { Handle_QuitToMainMenuRequest(quit); }));

		m_EventHandles.push_back(m_Gui.RequestResourcePackChange.Subscribe(
			[this](const std::string& resourcePackName) { Handle_ResourcePackChangeRequest(resourcePackName); }));
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

	void Renderer::Handle_StartMultiplayerGameRequest(const Gui::MultiplayerGameStartInfo& startInfo)
	{
		RequestStartMultiplayerGame.Trigger(startInfo);

		// Enable mouse capture for gameplay
		m_InputsManager.SetMouseCaptureEnabled(true);

		m_IsPaused = false;
	}

	void Renderer::Handle_BackToGameRequest()
	{
		PauseGame(false);
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

	void Renderer::Handle_ResourcePackChangeRequest(const std::string& resourcePackName)
	{
		EngineContext::Get().Assets->SetCurrentResourcePack(resourcePackName);

		// Reload everything that uses assets
		m_Gui.ReloadTextures();
		m_WorldRenderer.ReloadTextures();
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

		ImGui::Text("Client Version: %s", CLIENT_VERSION.c_str());

		// ----- Server Info Debug -----
		if (ImGui::CollapsingHeader("Server Info", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (m_ServerInfo)
			{
				ImGui::Text("Name: %s", m_ServerInfo->Name.c_str());
				ImGui::Text("Address: %s", m_ServerInfo->Address.c_str());
				ImGui::Text("Port: %d", m_ServerInfo->Port);
				ImGui::Text("Simu Dist: %d", m_ServerInfo->SimulationDistance);
			}
			else
			{
				ImGui::Text("No server info available");
			}
		}

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
