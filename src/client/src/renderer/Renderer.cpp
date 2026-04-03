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
		  m_WorldRenderer(worldManager, m_Camera), m_KeyBinds(m_InputsManager), m_PhysicsEngine(*worldManager),
		  m_EntityRenderer(m_Camera)
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

			// Render GUI
			m_Gui.Render();

			// Render Debug Panels
			RenderDebugPanel();
			RenderPhysicsDebugPanel();

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

		// ----- Apply Controls Settings -----
		const auto& controls = settings.Controls;
		if (args.FOV_Changed)
		{
			m_Camera->SetFovY(controls.FOV);
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
	}

	void Renderer::ProcessGameplayInputs()
	{
		// ----- RAYCASTING TO DETECT BLOCKS -----
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

		m_HitBlock = hitBlock;

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

		// Constants
		// Ground
		float groundMaxSpeed = 6.0f;
		float groundAcceleration = 120.0f;
		float groundDeceleration = 150.f;

		// Air
		float airMaxSpeed = 5.0f;
		float airAcceleration = 8.0f;
		float airDeceleration = 4.0f; // optional, often low

		// Flying
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
		KeyState moveDownKeyState = m_KeyBinds.GetKeyState(eAction::Sneak);
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
		if (toggleFlyModeKeyState.IsDoublePressed)
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
			if (moveDownKeyState.IsPressed)
				moveDir -= Up;

			if (glm::length(moveDir) > 0.0f)
				moveDir = glm::normalize(moveDir);

			if (speedUpKeyState.IsPressed)
				flyMaxSpeed *= 2.0f;

			glm::vec3 desiredVelocity = moveDir * flyMaxSpeed;

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

			float maxSpeed = physics.OnGround ? groundMaxSpeed : airMaxSpeed;
			float acceleration = physics.OnGround ? groundAcceleration : airAcceleration;
			float deceleration = physics.OnGround ? groundDeceleration : airDeceleration;

			if (moveForwardKeyState.IsPressed)
				moveDir += frontXZ;
			if (moveBackwardKeyState.IsPressed)
				moveDir -= frontXZ;
			if (moveLeftKeyState.IsPressed)
				moveDir -= glm::normalize(glm::cross(frontXZ, Up));
			if (moveRightKeyState.IsPressed)
				moveDir += glm::normalize(glm::cross(frontXZ, Up));

			if (glm::length(moveDir) > 0.0f)
			{
				glm::vec3 desiredDir = glm::normalize(moveDir);

				player->SetState(Entity::State::Walking);

				float sprintFactor = 1.0f;

				if (speedUpKeyState.IsPressed)
				{
					player->SetState(Entity::State::Running);

					// How aligned we are with forward direction
					float forwardDot = glm::dot(desiredDir, frontXZ);

					// Only boost if moving forward
					if (forwardDot > 0.7f) // ~45° cone (tweakable)
					{
						sprintFactor = 1.5f;
					}
				}

				float finalMaxSpeed = maxSpeed * sprintFactor;
				float finalAcceleration = acceleration * sprintFactor;

				physics.Velocity = MoveTowards(
					physics.Velocity, desiredDir * finalMaxSpeed, finalAcceleration * static_cast<float>(m_DeltaTime));
			}
			else
			{
				physics.Velocity =
					MoveTowards(physics.Velocity, glm::vec3(0.0f), deceleration * static_cast<float>(m_DeltaTime));
				player->SetState(Entity::State::Idle);
			}

			// Jumping
			if (moveUpKeyState.IsPressed && physics.OnGround)
			{
				physics.Velocity.y = m_PhysicsEngine.GetJumpStrength();

				glm::vec3 horizontalVelocity = glm::vec3(physics.Velocity.x, 0.0f, physics.Velocity.z);
				float horizontalSpeed = glm::length(horizontalVelocity);

				if (horizontalSpeed > 0.0001f)
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
						physics.Velocity += frontXZ * (groundMaxSpeed / 3);
					}
					else
					{
						// Snap for large angles, but reduce speed
						float penalty = 0.7f; //

						physics.Velocity.x = desiredDir.x * horizontalSpeed * penalty;
						physics.Velocity.z = desiredDir.z * horizontalSpeed * penalty;
					}
				}

				physics.OnGround = false;
			}
		}

		player->SetPhysicsBody(physics);
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
	}

	void Renderer::SubscribeToGuiEvents()
	{
		m_EventHandles.push_back(m_Gui.RequestCursorStyleChange.Subscribe([this](const CursorStyle& style)
																		  { Handle_CursorStyleChangeRequest(style); }));

		m_EventHandles.push_back(m_Gui.RequestStartSingleplayerGame.Subscribe(
			[this](const WorldInfos& worldInfos) { Handle_StartSingleplayerGameRequest(worldInfos); }));

		m_EventHandles.push_back(m_Gui.RequestStartMultiplayerGame.Subscribe(
			[this](const Gui::MultiplayerGameStartInfo& startInfo) { Handle_StartMultiplayerGameRequest(startInfo); }));

		m_EventHandles.push_back(m_Gui.RequestBackToGame.Subscribe([this](bool) { Handle_BackToGameRequest(); }));

		m_EventHandles.push_back(
			m_Gui.RequestQuitToMainMenu.Subscribe([this](bool quit) { Handle_QuitToMainMenuRequest(quit); }));

		m_EventHandles.push_back(m_Gui.RequestResourcePackChange.Subscribe(
			[this](const std::string& resourcePackName) { Handle_ResourcePackChangeRequest(resourcePackName); }));

		m_EventHandles.push_back(m_Gui.UserSettingsChanged.Subscribe([this](const UserSettingsChangedEventArgs& args)
																	 { Handle_UserSettingsChanged(args); }));
	}

	void Renderer::Handle_CursorStyleChangeRequest(const CursorStyle& style)
	{
		m_InputsManager.SetCursorStyle(style);
	}

	void Renderer::Handle_StartSingleplayerGameRequest(const WorldInfos& worldInfos)
	{
		RequestStartSingleplayerGame.Trigger(worldInfos);

		// Enable mouse capture for gameplay
		m_InputsManager.SetMouseCaptureEnabled(true);

		m_Gui.SetIsInGame(true);

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

			m_Gui.SetIsInGame(false);

			m_IsPaused = false;
		}
	}

	void Renderer::Handle_ResourcePackChangeRequest(const std::string& resourcePackName)
	{
		EngineContext::Get().Assets->SetCurrentResourcePack(resourcePackName);

		// Reload everything that uses assets
		m_Gui.ReloadTextures();
		m_WorldRenderer.ReloadTextures();
		m_EntityRenderer.ReloadTextures();
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
				m_Camera->SetFovY(fov);

			if (ImGui::DragFloat("Aspect Ratio", &aspect, 0.01f))
				m_Camera->SetAspectRatio(aspect);

			ImGui::DragFloat("Speed", &m_CameraSpeed, 0.1f, 0.1f, 50.f);
		}

		// ----- Raycast Debug -----
		if (ImGui::CollapsingHeader("Raycast", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Text(
				"Hit Block Position: %d, %d, %d", m_HitBlock.Position.x, m_HitBlock.Position.y, m_HitBlock.Position.z);
			ImGui::Text("Hit Block ID: %d", m_HitBlock.ID());
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

		ImGui::End();
	}

	void Renderer::EndImGuiFrame()
	{
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	void Renderer::CleanupOpenGl() {}

} // namespace onion::voxel
