#include "Renderer.hpp"

#include <iostream>
#include <stdexcept>
#include <stop_token>
#include <thread>

#include "version.hpp"

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
		  m_WorldRenderer(worldManager, m_Camera), m_KeyBinds(m_InputsManager), m_PhysicsEngine(*worldManager),
		  m_EntityRenderer(m_Camera), m_FovSmoother(m_Camera)
	{
		// Setup Timer Save UserSettings
		m_TimerDelayedSaveUserSettings.setTimeoutFunction([this]() { SaveUserSettings(); });
		m_TimerDelayedSaveUserSettings.setElapsedPeriod(std::chrono::milliseconds(1000));
		m_TimerDelayedSaveUserSettings.setRepeat(false);

		// Load User Settings
		std::filesystem::path userSettingsPath = GetUserSettingsPath();
		UserSettings settings = UserSettings::Load(userSettingsPath);

		if (!std::filesystem::exists(userSettingsPath))
		{
			std::cerr << "UserSettings file not found at " << userSettingsPath
					  << ". A new file will be created with default settings.\n";
			UserSettings::Save(settings, userSettingsPath);
		}

		// Sets the Engine Context
		EngineContext::Initialize(worldManager.get(), &m_AssetsManager, &m_InputsManager, settings);

		UserSettingsChangedEventArgs args(settings, true);

		// Dirty Tick to avoid crash on first load because OpenGL context is not created yet.
		args.ResourcePack_Changed = false;
		EngineContext::Get().Assets->SetCurrentResourcePack(settings.ResourcePack);

		ApplyUserSettings(args);
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

	void Renderer::SetServerInfo(std::shared_ptr<ServerInfo> serverInfo)
	{
		m_ServerInfo = serverInfo;
	}

	std::shared_ptr<ServerInfo> Renderer::GetServerInfo() const
	{
		return m_ServerInfo;
	}

	uint8_t Renderer::GetRenderDistance() const
	{
		return m_WorldManager->GetChunkPersistanceDistance();
	}

	void Renderer::SetPlayerUUID(const std::string& uuid)
	{
		m_PlayerUUID = uuid;
		EngineContext::Get().PlayerUUID = uuid;
	}

	void Renderer::InitWindow()
	{
		auto settings = EngineContext::Get().Settings();

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

		bool vsyncEnabled = settings.Video.VSyncEnabled;
		glfwSwapInterval(vsyncEnabled ? 1 : 0);

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

	void Renderer::InitOpenGl()
	{
		// ----- OpenGL State -----
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDisable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// ----- OpenGL Buffers -----
		glGenFramebuffers(1, &m_SceneFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, m_SceneFBO);

		// Color texture
		glGenTextures(1, &m_SceneColorTexture);
		glBindTexture(GL_TEXTURE_2D, m_SceneColorTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_WindowWidth, m_WindowHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_SceneColorTexture, 0);

		// Depth buffer
		glGenRenderbuffers(1, &m_DepthRenderBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, m_DepthRenderBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_WindowWidth, m_WindowHeight);

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_DepthRenderBuffer);

		// Check
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cerr << "Framebuffer not complete!" << std::endl;

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// Framebuffers for blurring
		for (int i = 0; i < 2; i++)
		{
			glGenFramebuffers(1, &m_BlurFBO[i]);
			glBindFramebuffer(GL_FRAMEBUFFER, m_BlurFBO[i]);

			glGenTextures(1, &m_BlurTexture[i]);
			glBindTexture(GL_TEXTURE_2D, m_BlurTexture[i]);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_WindowWidth, m_WindowHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_BlurTexture[i], 0);
		}
	}

	void Renderer::RenderThreadFunction(std::stop_token st)
	{
		m_IsRunning.store(true);

		InitWindow();

		InitOpenGl();

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

			// Draws the 3D Scene to the FBO
			RenderSceneToFBO();

			GLuint finalTexture = m_SceneColorTexture;
			eMenu activeMenu = m_Gui.GetActiveMenu();
			bool isMainMenu = activeMenu == eMenu::MainMenu;
			bool isInGameplay = activeMenu == eMenu::Gameplay;
			bool blurry = (!isMainMenu && !isInGameplay);
			if (blurry)
			{
				finalTexture = ApplyBlur(m_SceneColorTexture);
			}

			// Draws the FBO texture to the screen
			PresentScene(finalTexture);

			// Render GUI
			m_Gui.Render();

			// Render Debug Panels
			if (EngineContext::Get().ShowDebugMenus)
			{
				RenderDebugPanel();
				RenderPhysicsDebugPanel();
			}

			// End ImGui Frame
			EndImGuiFrame();

			// Swap buffers and poll events
			glfwSwapBuffers(m_Window);
			glfwPollEvents();

			// Cap FPS if needed
			if (!m_IsVsyncEnabled && m_MaxFps > 0)
			{
				double targetFrameTime = 1.0 / static_cast<double>(m_MaxFps);
				CapFPS(targetFrameTime);
			}
		}

		// Cleanup
		CleanupOpenGl();

		m_Gui.Shutdown();
		m_InputsManager.Delete();
		m_WorldRenderer.Unload();
		m_EntityRenderer.Unload();

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

	void Renderer::CapFPS(double targetFrameTime)
	{
		using clock = std::chrono::high_resolution_clock;

		static auto lastTime = clock::now();

		auto now = clock::now();
		double elapsed = std::chrono::duration<double>(now - lastTime).count();

		double remaining = targetFrameTime - elapsed;

		if (remaining > 0.0)
		{
			// Sleep most of it (leave a small margin)
			if (remaining > 0.002) // 2 ms safety margin
			{
				std::this_thread::sleep_for(std::chrono::duration<double>(remaining - 0.001));
			}

			// Busy wait for precision
			while (std::chrono::duration<double>(clock::now() - lastTime).count() < targetFrameTime)
			{
				// spin
			}
		}

		lastTime = clock::now();
	}

	std::filesystem::path Renderer::GetUserSettingsPath() const
	{
		return Utils::GetExecutableDirectory() / "user_settings.json";
	}

	void Renderer::ApplyUserSettings(const UserSettingsChangedEventArgs& args)
	{
		EngineContext::Get().UpdateSettings(args.NewSettings);

		const auto& settings = args.NewSettings;

		// ----- Apply Last Selected Resource Pack -----
		if (args.ResourcePack_Changed)
		{
			std::string currentPack = m_AssetsManager.GetCurrentResourcePack();
			std::string newlySelectedPack = settings.ResourcePack;
			if (currentPack != newlySelectedPack)
			{
				EngineContext::Get().Assets->SetCurrentResourcePack(newlySelectedPack);

				// Reload everything that uses assets
				m_Gui.ReloadTextures();
				m_WorldRenderer.ReloadTextures();
				m_EntityRenderer.ReloadTextures();
			}
		}

		// ----- Apply Controls Settings -----
		const auto& controls = settings.Controls;
		if (args.FOV_Changed)
		{
			m_Camera->SetFov(controls.FOV);
		}

		if (args.MouseSensitivity_Changed)
		{
			m_InputsManager.SetMouseSensitivity(controls.mouseSettings.Sensitivity);
		}

		if (args.MouseScrollSensitivity_Changed)
		{
			m_InputsManager.SetMouseScrollSensitivity(controls.mouseSettings.ScrollSensitivity);
		}

		if (args.VSyncEnabled_Changed)
		{
			m_IsVsyncEnabled = settings.Video.VSyncEnabled;
			glfwSwapInterval(m_IsVsyncEnabled ? 1 : 0);
		}

		if (args.MaxFPS_Changed)
		{
			m_MaxFps = settings.Video.MaxFPS;
		}

		if (args.RenderDistance_Changed)
		{
			m_WorldManager->SetChunkPersistanceDistance(settings.Video.RenderDistance);
			EvtRenderDistanceChanged.Trigger(settings.Video.RenderDistance);
		}

		if (args.SimulationDistance_Changed)
		{
			//m_WorldManager->SetSimulationDistance(settings.Video.SimulationDistance);
		}

		if (args.KeyBinds_Changed)
		{
			RegisterInputs();
		}

		// Restart the Save timer : Will be saved in 1s if no other setting is changed
		m_TimerDelayedSaveUserSettings.Restart();
	}

	void Renderer::SaveUserSettings()
	{
		std::filesystem::path userSettingsPath = GetUserSettingsPath();
		EngineContext::Get().SaveSettings(userSettingsPath);
	}

	void Renderer::SubscribeToInputsManagerEvents()
	{
		m_InputsManagerEventHandles.push_back(m_InputsManager.EvtFramebufferResized.Subscribe(
			[this](const FramebufferState& framebufferState) { Handle_FramebufferResized(framebufferState); }));
	}

	void Renderer::Handle_FramebufferResized(const FramebufferState& framebufferState)
	{
		m_WindowWidth = framebufferState.Width;
		m_WindowHeight = framebufferState.Height;

		if (m_WindowWidth == 0 || m_WindowHeight == 0)
			return;

		glViewport(0, 0, m_WindowWidth, m_WindowHeight);

		// Resize color texture
		glBindTexture(GL_TEXTURE_2D, m_SceneColorTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_WindowWidth, m_WindowHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
		glBindTexture(GL_TEXTURE_2D, 0);

		// Resize depth buffer
		glBindRenderbuffer(GL_RENDERBUFFER, m_DepthRenderBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_WindowWidth, m_WindowHeight);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		// Resize blur textures
		for (int i = 0; i < 2; i++)
		{
			glBindTexture(GL_TEXTURE_2D, m_BlurTexture[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_WindowWidth, m_WindowHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		DebugDraws::ScreenWidth = m_WindowWidth;
		DebugDraws::ScreenHeight = m_WindowHeight;

		m_Camera->SetAspectRatio(static_cast<float>(m_WindowWidth) / static_cast<float>(m_WindowHeight));
	}

	void Renderer::RegisterInputs()
	{
		const UserSettings settings = EngineContext::Get().Settings();
		const auto actionToKey = settings.Controls.keyBindsSettings.ActionToKey;

		InputConfig everyFrame(false, 0, 0, 0);
		InputConfig noRepeat(true, 9999999, 0, 0);
		InputConfig repeatWithDelay(true, 0.6f, 0.4f, 0.5f);

		m_KeyBinds.RemapAction(eAction::WalkForward, actionToKey.at(eAction::WalkForward), everyFrame);
		m_KeyBinds.RemapAction(eAction::WalkBackward, actionToKey.at(eAction::WalkBackward), everyFrame);
		m_KeyBinds.RemapAction(eAction::StrafeLeft, actionToKey.at(eAction::StrafeLeft), everyFrame);
		m_KeyBinds.RemapAction(eAction::StrafeRight, actionToKey.at(eAction::StrafeRight), everyFrame);
		m_KeyBinds.RemapAction(eAction::Jump, actionToKey.at(eAction::Jump), everyFrame);
		m_KeyBinds.RemapAction(eAction::Sneak, actionToKey.at(eAction::Sneak), everyFrame);
		m_KeyBinds.RemapAction(eAction::Sprint, actionToKey.at(eAction::Sprint), everyFrame);

		m_KeyBinds.RemapAction(eAction::ToggleMouseCapture, actionToKey.at(eAction::ToggleMouseCapture), noRepeat);
		m_KeyBinds.RemapAction(eAction::Pause, actionToKey.at(eAction::Pause), noRepeat);
		m_KeyBinds.RemapAction(eAction::CloseMenu, actionToKey.at(eAction::CloseMenu), noRepeat);
		m_KeyBinds.RemapAction(eAction::ToggleDebugMenus, actionToKey.at(eAction::ToggleDebugMenus), noRepeat);

		m_KeyBinds.RemapAction(eAction::Attack, actionToKey.at(eAction::Attack), repeatWithDelay);
		m_KeyBinds.RemapAction(eAction::Interact, actionToKey.at(eAction::Interact), repeatWithDelay);
		m_KeyBinds.RemapAction(eAction::ToggleFlyMode, actionToKey.at(eAction::ToggleFlyMode), repeatWithDelay);
	}

	void Renderer::ProcessInputs()
	{
		KeyState toggleMouseCaptureKeyState = m_KeyBinds.GetKeyState(eAction::ToggleMouseCapture);
		if (toggleMouseCaptureKeyState.IsPressed)
		{
			bool mouseCapture = m_InputsManager.IsMouseCaptureEnabled();
			m_InputsManager.SetMouseCaptureEnabled(!mouseCapture);
		}

		KeyState toggleDebugMenusKeyState = m_KeyBinds.GetKeyState(eAction::ToggleDebugMenus);
		if (toggleDebugMenusKeyState.IsPressed)
		{
			auto& engineContext = EngineContext::Get();
			engineContext.ShowDebugMenus = !engineContext.ShowDebugMenus;
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

		if (m_RenderState == eRenderState::InGame && !m_IsPaused)
		{
			if (m_IsFreeCamera)
			{
				UpdateCameraFromInputs();
			}
			else
			{
				UpdatePlayerFromInputs();
				ProcessGameplayInputs();
			}
		}

		if (m_RenderState == eRenderState::InGame && !m_IsFreeCamera)
		{
			// Stick the camera to the player
			std::shared_ptr<Player> player = GetPlayer();
			if (player)
			{
				m_Camera->SetPosition(player->GetEyePosition());
				m_Camera->SetFront(player->GetFacing());
			}
		}

		// Update FOV Smoother
		m_FovSmoother.Update(static_cast<float>(m_DeltaTime));
	}

	void Renderer::ProcessGameplayInputs()
	{
		// ----- RAYCASTING TO DETECT BLOCKS -----
		glm::vec3 rayOrigin = m_Camera->GetPosition();
		glm::vec3 rayDirection = m_Camera->GetFront();

		m_CurrentRaycastHit = Raycaster::Raycast(*m_WorldManager, rayOrigin, rayDirection, 10.0f, 300);

		KeyState attackKeyState = m_KeyBinds.GetKeyState(eAction::Attack);
		if (attackKeyState.IsPressed && m_CurrentRaycastHit.has_value())
		{
			const Block& hitBlock = m_CurrentRaycastHit->HitBlock;
			Block airBlockToPlace = Block(hitBlock.Position, BlockState(BlockId::Air));

			bool success = m_WorldManager->SetBlock(
				airBlockToPlace, WorldManager::BlocksChangedEventArgs::eOrigin::PlayerAction, true);

			std::cout << "Attempting to remove block at " << hitBlock.Position.x << ", " << hitBlock.Position.y << ", "
					  << hitBlock.Position.z << " - Success: " << (success ? "Yes" : "No") << std::endl;
		}

		KeyState interactKeyState = m_KeyBinds.GetKeyState(eAction::Interact);
		if (interactKeyState.IsPressed && m_CurrentRaycastHit.has_value())
		{
			const Block& adjacentBlock = m_CurrentRaycastHit->AdjacentBlock;
			Block blockToPlace = Block(adjacentBlock.Position, BlockState(BlockId::BirchStairs));

			bool success = m_WorldManager->SetBlock(
				blockToPlace, WorldManager::BlocksChangedEventArgs::eOrigin::PlayerAction, true);

			std::cout << "Attempting to place block at " << blockToPlace.Position.x << ", " << blockToPlace.Position.y
					  << ", " << blockToPlace.Position.z << " - Success: " << (success ? "Yes" : "No") << std::endl;
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

	std::shared_ptr<Player> Renderer::GetPlayer() const
	{
		if (m_PlayerUUID.empty())
		{
			return nullptr;
		}

		return m_WorldManager->GetPlayer(m_PlayerUUID);
	}

	glm::vec3 Renderer::GetPlayerPosition() const
	{
		std::shared_ptr<Player> player = GetPlayer();
		if (player)
		{
			return player->GetPosition();
		}
		return glm::vec3(0.0f);
	}

	namespace
	{
		static glm::vec3 MoveTowards(const glm::vec3& current, const glm::vec3& target, float maxDelta)
		{
			glm::vec3 delta = target - current;
			float len = glm::length(delta);

			if (len <= maxDelta || len == 0.0f)
				return target;

			return current + (delta / len) * maxDelta;
		}
	} // namespace

	void Renderer::UpdatePlayerFromInputs()
	{
		std::shared_ptr<Player> player = GetPlayer();

		// No player found, skip processing
		if (!player)
		{
			return;
		}

		// If mouse capture is not enabled, skip player movement processing
		if (!m_InputsManager.IsMouseCaptureEnabled())
		{
			return;
		}

		// Flying Constants
		float flyMaxSpeed = m_PlayerFlySpeed;
		float flyAcceleration = m_PlayerFlySpeed * 5.f;
		float flyDeceleration = m_PlayerFlySpeed * 5.f;

		auto inputs = m_InputsManager.GetInputsSnapshot();

		auto physics = player->GetPhysicsBody();

		glm::vec3 playerFront = glm::normalize(player->GetFacing());

		// ----- KEY STATES -----
		KeyState speedUpKeyState = m_KeyBinds.GetKeyState(eAction::Sprint);
		KeyState moveForwardKeyState = m_KeyBinds.GetKeyState(eAction::WalkForward);
		KeyState moveBackwardKeyState = m_KeyBinds.GetKeyState(eAction::WalkBackward);
		KeyState moveLeftKeyState = m_KeyBinds.GetKeyState(eAction::StrafeLeft);
		KeyState moveRightKeyState = m_KeyBinds.GetKeyState(eAction::StrafeRight);
		KeyState moveUpKeyState = m_KeyBinds.GetKeyState(eAction::Jump);
		KeyState sneakKeyState = m_KeyBinds.GetKeyState(eAction::Sneak);
		KeyState toggleFlyModeKeyState = m_KeyBinds.GetKeyState(eAction::ToggleFlyMode);

		// ----- PLAYER ORIENTATION -----
		if (inputs->Mouse.MovementOffsetChanged)
		{
			const float xoffset = static_cast<float>(inputs->Mouse.Xoffset);
			const float yoffset = static_cast<float>(inputs->Mouse.Yoffset);

			// Extract yaw/pitch from current direction vector
			float yaw = glm::degrees(std::atan2(playerFront.z, playerFront.x));
			float pitch = glm::degrees(std::asin(playerFront.y));

			// Apply mouse deltas
			yaw += xoffset;
			pitch += yoffset;

			// Clamp vertical look
			pitch = glm::clamp(pitch, -89.0f, 89.0f);

			// Rebuild normalized direction vector
			glm::vec3 newFront;
			newFront.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
			newFront.y = std::sin(glm::radians(pitch));
			newFront.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));

			player->SetFacing(glm::normalize(newFront));
			playerFront = glm::normalize(newFront);
		}

		// ----- TOGGLE FLYING MODE -----
		if (m_AllowFlyToggle && toggleFlyModeKeyState.IsDoublePressed)
		{
			physics.IsFlying = !physics.IsFlying;
			if (physics.IsFlying)
			{
				std::cout << "Flying mode enabled\n";
				// Reset vertical velocity
				physics.Velocity.y = 0.0f;
			}
			else
			{
				std::cout << "Flying mode disabled\n";
			}
		}

		// Disable flying when touching ground
		if (physics.IsFlying && physics.OnGround)
		{
			physics.IsFlying = false;
		}

		// ----- PLAYER FLYING SPEED -----
		float flyVelocity = m_PlayerFlySpeed * (float) m_DeltaTime;
		if (physics.IsFlying && inputs->Mouse.ScrollOffsetChanged)
		{
			const double yoffset = inputs->Mouse.ScrollYoffset;
			if (yoffset != 0.f)
			{
				float coeefIncrease = 1.3f;
				float coeefDecrease = 0.7f;
				if (yoffset > 0)
				{
					m_PlayerFlySpeed *= coeefIncrease; // Speed up
				}
				else if (yoffset < 0)
				{
					m_PlayerFlySpeed *= coeefDecrease; // Slow down
				}
			}

			if (speedUpKeyState.IsPressed)
			{
				flyVelocity *= 2.0f; // Double speed if left control is pressed
			}
		}

		// Flatten the front vector for XZ movement
		glm::vec3 frontXZ = glm::normalize(glm::vec3(playerFront.x, 0.0f, playerFront.z));
		const glm::vec3 Up(0.0f, 1.0f, 0.0f); // Up vector

		glm::vec3 moveDir(0.0f);

		// ----- MOVEMENT -----
		player->SetIsSneaking(sneakKeyState.IsPressed);

		if (physics.IsFlying)
		{

			// Flying movement
			if (moveForwardKeyState.IsPressed)
				moveDir += frontXZ;
			if (moveBackwardKeyState.IsPressed)
				moveDir -= frontXZ;
			if (moveLeftKeyState.IsPressed)
				moveDir -= glm::normalize(glm::cross(frontXZ, Up));
			if (moveRightKeyState.IsPressed)
				moveDir += glm::normalize(glm::cross(frontXZ, Up));
			if (moveUpKeyState.IsPressed)
				moveDir += Up;
			if (sneakKeyState.IsPressed)
				moveDir -= Up;

			if (glm::length(moveDir) > 0.0f)
				moveDir = glm::normalize(moveDir);

			if (speedUpKeyState.IsPressed)
				flyMaxSpeed *= 2.0f;

			glm::vec3 desiredVelocity = moveDir * m_PlayerFlySpeed;

			if (glm::length(moveDir) > 0.0f)
			{
				Entity::State state = speedUpKeyState.IsPressed ? Entity::State::Running : Entity::State::Walking;
				player->SetState(state);

				physics.Velocity =
					MoveTowards(physics.Velocity, desiredVelocity, flyAcceleration * static_cast<float>(m_DeltaTime));
			}
			else
			{
				player->SetState(Entity::State::Idle);
				physics.Velocity =
					MoveTowards(physics.Velocity, glm::vec3(0.0f), flyDeceleration * static_cast<float>(m_DeltaTime));
			}
		}
		else
		{
			// Walking movement

			float maxSpeed = physics.OnGround ? m_GroundMaxSpeed : m_AirMaxSpeed;
			if (player->IsSneaking())
			{
				maxSpeed *= m_SneakSpeedFactor;
			}

			float acceleration = physics.OnGround ? m_GroundAcceleration : m_AirAcceleration;

			if (moveForwardKeyState.IsPressed)
				moveDir += frontXZ;
			if (moveBackwardKeyState.IsPressed)
				moveDir -= frontXZ;
			if (moveLeftKeyState.IsPressed)
				moveDir -= glm::normalize(glm::cross(frontXZ, Up));
			if (moveRightKeyState.IsPressed)
				moveDir += glm::normalize(glm::cross(frontXZ, Up));

			// Normalize early so each axis component represents its true proportion
			// of the intended direction (length = 1 for pure cardinal, < 1 after any
			// sneak edge clamp removes an axis, naturally encoding the speed reduction).
			if (glm::length(moveDir) > 0.0f)
				moveDir = glm::normalize(moveDir);

			// --- Sneak edge prevention ---
			// Test each world-space horizontal axis independently after moveDir is
			// normalized. Zeroing a clamped axis reduces moveDir's length below 1,
			// which proportionally reduces the MoveTowards target speed with no extra
			// bookkeeping (sneakEdgeSpeedRatio removed).
			if (player->IsSneaking() && physics.OnGround)
			{
				const glm::vec3 currentPos = player->GetTransform().Position;

				if (std::abs(moveDir.x) > 0.0f || std::abs(physics.Velocity.x) > 0.0f)
				{
					float deltaX = moveDir.x * maxSpeed * static_cast<float>(m_DeltaTime) +
						physics.Velocity.x * static_cast<float>(m_DeltaTime);
					glm::vec3 testPos = currentPos + glm::vec3(deltaX, 0.0f, 0.0f);
					if (!m_PhysicsEngine.HasGroundSupport(testPos, physics.HalfSize, physics.Offset))
					{
						moveDir.x = 0.0f;
						physics.Velocity.x = 0.0f;
					}
				}

				if (std::abs(moveDir.z) > 0.0f || std::abs(physics.Velocity.z) > 0.0f)
				{
					float deltaZ = moveDir.z * maxSpeed * static_cast<float>(m_DeltaTime) +
						physics.Velocity.z * static_cast<float>(m_DeltaTime);
					glm::vec3 testPos = currentPos + glm::vec3(0.0f, 0.0f, deltaZ);
					if (!m_PhysicsEngine.HasGroundSupport(testPos, physics.HalfSize, physics.Offset))
					{
						moveDir.z = 0.0f;
						physics.Velocity.z = 0.0f;
					}
				}
			}

			if (glm::length(moveDir) > 0.0f)
			{
				player->SetState(Entity::State::Walking);

				float sprintFactor = 1.0f;

				// Can run only if sprint key is held, moving forward, and not sneaking
				if (speedUpKeyState.IsPressed && moveForwardKeyState.IsPressed && !player->IsSneaking())
				{
					player->SetState(Entity::State::Running);

					// How aligned we are with forward direction
					float forwardDot = glm::dot(moveDir, frontXZ);

					// Only boost if moving forward
					if (forwardDot > 0.7f) // ~45° cone (tweakable)
					{
						sprintFactor = 1.5f;
					}
				}

				float finalMaxSpeed = maxSpeed * sprintFactor;
				float finalAcceleration = acceleration * sprintFactor;

				physics.Velocity = MoveTowards(
					physics.Velocity, moveDir * finalMaxSpeed, finalAcceleration * static_cast<float>(m_DeltaTime));
			}
			else
			{
				// Use a higher deceleration when airborne and no key is pressed so that
				// the jump horizontal kick doesn't cause uncontrolled drift after release.
				float releaseDecel = physics.OnGround ? m_GroundDeceleration : m_JumpReleaseDeceleration;

				// Only apply release deceleration to XZ — vel.y is owned by gravity.
				glm::vec3 target(0.0f, physics.Velocity.y, 0.0f);
				physics.Velocity =
					MoveTowards(physics.Velocity, target, releaseDecel * static_cast<float>(m_DeltaTime));
				player->SetState(Entity::State::Idle);
			}

			// Jumping
			constexpr float kJumpCooldownDuration = 0.4f;

			// Coyote time: track when the player leaves the ground naturally (not from jumping).
			// m_WasOnGround is updated at the end of this block so we catch the exact frame
			// of the ground → air transition.
			bool onGroundNow = physics.OnGround;
			if (m_WasOnGround && !onGroundNow && m_CoyoteTimeRemaining <= 0.0f)
			{
				// Player just walked off an edge — open the coyote window
				m_CoyoteTimeRemaining = m_CoyoteTimeDuration;
			}
			else if (onGroundNow)
			{
				// Back on ground — reset window so it doesn't bleed into the next fall
				m_CoyoteTimeRemaining = 0.0f;
			}

			m_CoyoteTimeRemaining -= static_cast<float>(m_DeltaTime);

			// Tick down the jump cooldown each frame
			m_JumpCooldown -= static_cast<float>(m_DeltaTime);

			// Early cooldown reset: if the key was released since the last frame, allow
			// jumping again immediately so a deliberate re-press is never blocked.
			bool jumpKeyNowPressed = moveUpKeyState.IsPressed;
			if (m_JumpKeyWasPressed && !jumpKeyNowPressed)
			{
				m_JumpCooldown = 0.0f;
			}

			m_JumpKeyWasPressed = jumpKeyNowPressed;

			bool canJump = onGroundNow || m_CoyoteTimeRemaining > 0.0f;
			if (moveUpKeyState.IsPressed && canJump && m_JumpCooldown <= 0.0f)
			{
				// Reset vertical velocity before applying jump strength so coyote jumps
				// are not weakened by the downward velocity accumulated during the fall.
				physics.Velocity.y = m_PhysicsEngine.GetJumpStrength();

				if (m_CoyoteTimeRemaining > 0.0f)
				{
					std::cout << "Coyote Jump at " << m_CoyoteTimeRemaining << " seconds remaining\n";
				}

				// Consume the coyote window immediately to prevent it restarting
				// on the next frame when OnGround transitions false → false.
				m_CoyoteTimeRemaining = 0.0f;

				glm::vec3 horizontalVelocity = glm::vec3(physics.Velocity.x, 0.0f, physics.Velocity.z);
				float horizontalSpeed = glm::length(horizontalVelocity);

				if (horizontalSpeed > 0.0001f && moveForwardKeyState.IsPressed)
				{
					glm::vec3 currentDir = glm::normalize(horizontalVelocity);
					glm::vec3 desiredDir = glm::normalize(glm::vec3(playerFront.x, 0.0f, playerFront.z));

					float dot = glm::dot(currentDir, desiredDir);

					// Threshold ~90° (cos(90°) = 0)
					if (dot > 0.0f)
					{
						// Smooth blend for small angles
						float control = 0.3f;
						glm::vec3 newDir = glm::normalize(glm::mix(currentDir, desiredDir, control));

						physics.Velocity.x = newDir.x * horizontalSpeed;
						physics.Velocity.z = newDir.z * horizontalSpeed;

						// Add a small kick in facing direction for better feel
						physics.Velocity += frontXZ * (m_GroundMaxSpeed / 3);
					}
					else
					{
						// Snap for large angles, but reduce speed
						float penalty = 0.5f; //

						physics.Velocity.x = desiredDir.x * horizontalSpeed * penalty;
						physics.Velocity.z = desiredDir.z * horizontalSpeed * penalty;
					}
				}

				physics.OnGround = false;
				m_JumpCooldown = kJumpCooldownDuration;
			}

			m_WasOnGround = onGroundNow;
		}

		player->SetPhysicsBody(physics);

		// Update Player FOV based on state
		Entity::State currentState = player->GetState();
		const float configFov = EngineContext::Get().Settings().Controls.FOV;
		if (currentState == Entity::State::Running)
		{
			m_FovSmoother.SetTargetFov(configFov * m_FovRunningRatio);
		}
		else
		{
			m_FovSmoother.SetTargetFov(configFov);
		}
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
			const double xoffset = inputs->Mouse.Xoffset;
			const double yoffset = inputs->Mouse.Yoffset;

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

		KeyState moveForwardKeyState = m_KeyBinds.GetKeyState(eAction::WalkForward);
		KeyState moveBackwardKeyState = m_KeyBinds.GetKeyState(eAction::WalkBackward);
		KeyState moveLeftKeyState = m_KeyBinds.GetKeyState(eAction::StrafeLeft);
		KeyState moveRightKeyState = m_KeyBinds.GetKeyState(eAction::StrafeRight);
		KeyState moveUpKeyState = m_KeyBinds.GetKeyState(eAction::Jump);
		KeyState moveDownKeyState = m_KeyBinds.GetKeyState(eAction::Sneak);

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

		// Update Camera FOV based on state
		const float configFov = EngineContext::Get().Settings().Controls.FOV;
		if (speedUpKeyState.IsPressed)
		{
			m_FovSmoother.SetTargetFov(configFov * m_FovRunningRatio);
		}
		else
		{
			m_FovSmoother.SetTargetFov(configFov);
		}
	}

	void Renderer::SubscribeToGuiEvents()
	{
		m_EventHandles.push_back(m_Gui.EvtRequestCursorStyleChange.Subscribe(
			[this](const CursorStyle& style) { Handle_CursorStyleChangeRequest(style); }));

		m_EventHandles.push_back(m_Gui.EvtRequestStartSingleplayerGame.Subscribe(
			[this](const WorldInfos& worldInfos) { Handle_StartSingleplayerGameRequest(worldInfos); }));

		m_EventHandles.push_back(m_Gui.EvtRequestStartMultiplayerGame.Subscribe(
			[this](const ServerInfos& serverInfos) { Handle_StartMultiplayerGameRequest(serverInfos); }));

		m_EventHandles.push_back(m_Gui.EvtRequestBackToGame.Subscribe([this](bool) { Handle_BackToGameRequest(); }));

		m_EventHandles.push_back(
			m_Gui.EvtRequestQuitToMainMenu.Subscribe([this](bool quit) { Handle_QuitToMainMenuRequest(quit); }));

		m_EventHandles.push_back(m_Gui.EvtUserSettingsChanged.Subscribe([this](const UserSettingsChangedEventArgs& args)
																		{ Handle_UserSettingsChanged(args); }));
	}

	void Renderer::Handle_CursorStyleChangeRequest(const CursorStyle& style)
	{
		m_InputsManager.SetCursorStyle(style);
	}

	void Renderer::Handle_StartSingleplayerGameRequest(const WorldInfos& worldInfos)
	{
		EvtRequestStartSingleplayerGame.Trigger(worldInfos);

		// Enable mouse capture for gameplay
		m_InputsManager.SetMouseCaptureEnabled(true);

		m_Gui.SetIsInGame(true);

		m_IsPaused = false;
	}

	void Renderer::Handle_StartMultiplayerGameRequest(const ServerInfos& serverInfos)
	{
		EvtRequestStartMultiplayerGame.Trigger(serverInfos);

		// Enable mouse capture for gameplay
		m_InputsManager.SetMouseCaptureEnabled(true);

		m_Gui.SetIsInGame(true);

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
			EvtRequestQuitToMainMenu.Trigger(quit);

			m_WorldManager->RemoveAllChunks();
			m_WorldRenderer.DeleteChunkMeshes();

			m_CurrentRaycastHit = std::nullopt; // Reset HitBlock

			m_Gui.SetIsInGame(false);

			m_IsPaused = false;
		}
	}

	void Renderer::Handle_UserSettingsChanged(const UserSettingsChangedEventArgs& args)
	{
		//std::cout << "User settings changed. Applying new settings...\n";
		ApplyUserSettings(args);
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

		ImGui::Text("Client Version: %s", PROJECT_VERSION);

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
			float fov = m_Camera->GetFov();
			float aspect = m_Camera->GetAspectRatio();

			ImGui::Checkbox("Free Camera", &m_IsFreeCamera);

			if (ImGui::DragFloat3("Position", &position.x, 0.1f))
				m_Camera->SetPosition(position);

			if (ImGui::DragFloat3("Front", &front.x, 0.01f))
				m_Camera->SetFront(front);

			if (ImGui::DragFloat("Yaw", &yaw, 0.5f))
				m_Camera->SetYaw(yaw);

			if (ImGui::DragFloat("Pitch", &pitch, 0.5f, -89.f, 89.f))
				m_Camera->SetPitch(pitch);

			if (ImGui::DragFloat("FOV", &fov, 0.1f, 1.f, 120.f))
				m_Camera->SetFov(fov);

			if (ImGui::DragFloat("Aspect Ratio", &aspect, 0.01f))
				m_Camera->SetAspectRatio(aspect);

			ImGui::DragFloat("Speed", &m_CameraSpeed, 0.1f, 0.1f, 50.f);
		}

		// ----- Raycast Debug -----
		if (ImGui::CollapsingHeader("Raycast", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (m_CurrentRaycastHit.has_value())
			{
				const Block& hitBlock = m_CurrentRaycastHit->HitBlock;
				ImGui::Text(
					"Hit Block Position: %d, %d, %d", hitBlock.Position.x, hitBlock.Position.y, hitBlock.Position.z);
				ImGui::Text("Hit Block ID: %d", hitBlock.ID());
				ImGui::Text("Name: %s", BlockIds::GetName(hitBlock.ID()).c_str());
				int variantIndex = hitBlock.State.VariantIndex;
				ImGui::Text("Variant Index: %d", variantIndex);
			}
		}

		ImGui::End();
	}

	void Renderer::RenderPhysicsDebugPanel()
	{
		ImGui::Begin("Physics Debug");

		float gravity = m_PhysicsEngine.GetGravity();
		if (ImGui::DragFloat("Gravity", &gravity, 0.1f, -50.f, 50.f))
		{
			m_PhysicsEngine.SetGravity(gravity);
		}

		float jumpStrength = m_PhysicsEngine.GetJumpStrength();
		if (ImGui::DragFloat("Jump Strength", &jumpStrength, 0.1f, 0.f, 50.f))
		{
			m_PhysicsEngine.SetJumpStrength(jumpStrength);
		}

		ImGui::DragFloat("Coyote Time (s)", &m_CoyoteTimeDuration, 0.01f, 0.0f, 0.5f);

		ImGui::Checkbox("Allow Fly Toggle (double jump)", &m_AllowFlyToggle);

		ImGui::Separator();
		if (ImGui::CollapsingHeader("Ground Movement"))
		{
			ImGui::DragFloat("Max Speed##ground", &m_GroundMaxSpeed, 0.1f, 0.f, 50.f);
			ImGui::DragFloat("Acceleration##ground", &m_GroundAcceleration, 1.0f, 0.f, 500.f);
			ImGui::DragFloat("Deceleration##ground", &m_GroundDeceleration, 1.0f, 0.f, 500.f);
		}

		if (ImGui::CollapsingHeader("Air Movement"))
		{
			ImGui::DragFloat("Max Speed##air", &m_AirMaxSpeed, 0.1f, 0.f, 50.f);
			ImGui::DragFloat("Acceleration##air", &m_AirAcceleration, 0.5f, 0.f, 100.f);
			ImGui::DragFloat("Decel (key held)##air", &m_AirDeceleration, 0.5f, 0.f, 100.f);
			ImGui::DragFloat("Decel (key released)##air", &m_JumpReleaseDeceleration, 1.0f, 0.f, 200.f);
		}

		ImGui::End();
	}

	void Renderer::EndImGuiFrame()
	{
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	void Renderer::CleanupOpenGl()
	{
		// ----- Cleanup Shaders -----
		m_ScreenShader.Delete();
		m_BlurShader.Delete();

		// ----- Cleanup Buffers -----
		glDeleteFramebuffers(1, &m_SceneFBO);
		glDeleteTextures(1, &m_SceneColorTexture);
		glDeleteRenderbuffers(1, &m_DepthRenderBuffer);
	}

	void Renderer::RenderSceneToFBO()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_SceneFBO);
		glViewport(0, 0, m_WindowWidth, m_WindowHeight);

		glEnable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Render Raycasted block highlight
		if (m_CurrentRaycastHit.has_value())
		{
			//DebugDraws::DrawBlockOutline(m_HitBlock.Position, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), 3, true);
			//DebugDraws::DrawBlockOutline(prevBlockPos, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), 3, true);

			const Block& hitBlock = m_CurrentRaycastHit->HitBlock;
			const auto& blockId = hitBlock.ID();
			const auto& blockModels = BlockstateRegistry::Get();

			auto it = blockModels.find(blockId);
			if (it != blockModels.end())
			{
				const auto& variantModels = it->second;
				const VariantModel& variantModel = variantModels.at(hitBlock.State.VariantIndex);

				// Special case: cross-shaped models (e.g. grass, flowers).
				// Two thin elements both rotated 45° around Y — draw a single union AABB.
				const auto& elements = variantModel.Model.Elements;
				auto isThinY45 = [](const BlockModel::Element& e)
				{
					constexpr float kThinThreshold = 0.5f;
					bool thinOnX = (e.To.x - e.From.x) < kThinThreshold;
					bool thinOnZ = (e.To.z - e.From.z) < kThinThreshold;
					return (thinOnX || thinOnZ) && e.Rotation.Axis == "y" &&
						(e.Rotation.Angle == 45.f || e.Rotation.Angle == -45.f);
				};

				if (elements.size() == 2 && isThinY45(elements[0]) && isThinY45(elements[1]))
				{
					glm::vec3 unionFrom = glm::min(elements[0].From, elements[1].From);
					glm::vec3 unionTo = glm::max(elements[0].To, elements[1].To);

					const glm::vec3 worldFrom = glm::vec3(hitBlock.Position) + (unionFrom / 16.f);
					const glm::vec3 worldTo = glm::vec3(hitBlock.Position) + (unionTo / 16.f);

					DebugDraws::DrawWorldBoxMinMax(worldFrom, worldTo, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), 2, false);
				}
				else
				{
					for (const auto& element : elements)
					{
						const glm::vec3 worldFrom = glm::vec3(hitBlock.Position) + (element.From / 16.f);
						const glm::vec3 worldTo = glm::vec3(hitBlock.Position) + (element.To / 16.f);

						DebugDraws::DrawWorldBoxMinMax(worldFrom, worldTo, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), 2, false);
					}
				}

				//DebugDraws::DrawBlockOutline(hitBlock.Position, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), 3, true);
			}
			else
			{
				m_CurrentRaycastHit = std::nullopt; // Reset HitBlock if no model found.
			}
		}

		// Update and render the world and entities only when in the InGame state
		if (GetRenderState() == eRenderState::InGame)
		{
			if (!m_IsPaused)
			{
				m_WorldManager->RemoveDistantChunks();
				constexpr float maxDeltaTime = 1.f / 30.f; // Cap delta time to avoid big jumps
				float deltaTime = static_cast<float>(m_DeltaTime);
				m_PhysicsEngine.Update(std::min(deltaTime, maxDeltaTime));
			}

			m_WorldRenderer.Render();

			// Render the Player entity only when in freecam mode.
			std::vector<std::string> hiddenEntities;
			if (!m_IsFreeCamera)
			{
				hiddenEntities.push_back(m_PlayerUUID);
			}

			m_EntityRenderer.RenderEntities(hiddenEntities);
		}

		// Renger GUI Background (without ImGui elements) (so it can be blurred if needed)
		m_Gui.RenderBackground();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	GLuint Renderer::ApplyBlur(GLuint inputTexture)
	{
		bool horizontal = true;
		bool firstIteration = true;

		const int blurPasses = 6;

		m_BlurShader.Use();

		for (int i = 0; i < blurPasses; i++)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, m_BlurFBO[horizontal]);

			m_BlurShader.setBool("horizontal", horizontal);

			glActiveTexture(GL_TEXTURE0);

			if (firstIteration)
				glBindTexture(GL_TEXTURE_2D, inputTexture);
			else
				glBindTexture(GL_TEXTURE_2D, m_BlurTexture[!horizontal]);

			RenderFullscreenQuad();

			horizontal = !horizontal;

			if (firstIteration)
				firstIteration = false;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		return m_BlurTexture[!horizontal];
	}

	void Renderer::PresentScene(GLuint texture)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, m_WindowWidth, m_WindowHeight);

		glDisable(GL_DEPTH_TEST);

		m_ScreenShader.Use();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		m_ScreenShader.setInt("u_Texture", 0);

		RenderFullscreenQuad();
	}

	void Renderer::RenderFullscreenQuad()
	{
		static GLuint VAO = 0;
		static GLuint VBO = 0;

		if (VAO == 0)
		{
			float quadVertices[] = {// positions   // UVs
									-1.f, 1.f, 0.f, 1.f, -1.f, -1.f, 0.f, 0.f, 1.f, -1.f, 1.f, 0.f,

									-1.f, 1.f, 0.f, 1.f, 1.f,  -1.f, 1.f, 0.f, 1.f, 1.f,  1.f, 1.f};

			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);

			glBindVertexArray(VAO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) 0);

			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*) (2 * sizeof(float)));
		}

		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
	}

} // namespace onion::voxel
