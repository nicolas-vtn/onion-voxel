#include "inputs_manager.hpp"

using namespace onion::voxel;

void InputsManager::Init(GLFWwindow* window)
{

	m_Window = window;
	glfwSetWindowUserPointer(window, this);

	{
		std::unique_lock<std::mutex> lockFramebuffer(m_MutexFramebuffer);
		glfwGetFramebufferSize(m_Window, &m_FramebufferState.Width, &m_FramebufferState.Height);
		m_FramebufferState.Resized = true;
	}

	InitCallbacks();
}

void InputsManager::PoolInputs()
{

	{
		std::unique_lock<std::mutex> lockInputs(m_MutexSnapshot);
		m_InputsSnapshot = nullptr;
	}

	m_GlfwTime = glfwGetTime();

	PoolMouseMovement();
	PoolMouseInputs();
	PollKeyboardInputs();

	UpdateInputsSnapshot();

	ResetFlags();
}

void InputsManager::PoolMouseMovement()
{
	std::unique_lock<std::mutex> lock(m_MutexMouse);

	m_MouseState.CaptureEnabled = IsMouseCaptureEnabled();

	double xpos, ypos;
	glfwGetCursorPos(m_Window, &xpos, &ypos);

	m_MouseState.Xpos = xpos;
	m_MouseState.Ypos = ypos;

	if (!IsMouseCaptureEnabled())
	{
		return;
	}

	if (m_FirstMouse)
	{
		m_MouseLastX = xpos;
		m_MouseLastY = ypos;
		m_FirstMouse = false;
	}

	if (xpos == m_MouseLastX && ypos == m_MouseLastY)
	{
		// No movement
		m_MouseState.MovementOffsetChanged = false;
		m_MouseState.Xoffset = 0.0;
		m_MouseState.Yoffset = 0.0;
		return;
	}
	else
	{
		// Movement detected
		double xoffset = xpos - m_MouseLastX;
		double yoffset = m_MouseLastY - ypos; // reversed since y-coordinates range bottom to top

		m_MouseState.MovementOffsetChanged = true;
		m_MouseState.Xoffset = xoffset;
		m_MouseState.Yoffset = yoffset;
	}

	m_MouseLastX = xpos;
	m_MouseLastY = ypos;
}

void InputsManager::PoolMouseInputs()
{
	std::unique_lock<std::mutex> lock(m_MutexMouse);
	m_MouseState.LeftButtonPressed = glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
	m_MouseState.RightButtonPressed = glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
}

void InputsManager::PollKeyboardInputs()
{
	std::unique_lock<std::mutex> lock(m_MutexRegisteredInputs);
	for (auto& [inputId, keyControl] : m_RegisteredInputs)
	{
		bool isKeyDown = glfwGetKey(m_Window, static_cast<int>(keyControl.key)) == GLFW_PRESS;
		keyControl.Update(isKeyDown);
	}
}

void InputsManager::ResetFlags()
{
	{
		std::unique_lock<std::mutex> lockFramebuffer(m_MutexFramebuffer);
		m_FramebufferState.Resized = false;
	}

	{
		std::unique_lock<std::mutex> lock(m_MutexMouse);
		m_MouseState.ScrollOffsetChanged = false;
		m_MouseState.MovementOffsetChanged = false;
	}
}

std::shared_ptr<InputsSnapshot> InputsManager::GetInputsSnapshot()
{
	if (m_InputsSnapshot == nullptr)
	{
		throw std::runtime_error(
			"InputsSnapshot is not initialized. Make sure to call PoolInputs() before GetInputsSnapshot().");
	}
	return m_InputsSnapshot;
}

void InputsManager::InitCallbacks()
{
	glfwSetFramebufferSizeCallback(m_Window,
								   [](GLFWwindow* window, int width, int height)
								   {
									   // Retrieve the user pointer
									   auto* self = static_cast<InputsManager*>(glfwGetWindowUserPointer(window));
									   if (self)
										   self->FramebufferSizeCallback(width, height);
								   });

	glfwSetScrollCallback(m_Window,
						  [](GLFWwindow* window, double xoffset, double yoffset)
						  {
							  // Retrieve the user pointer
							  auto* self = static_cast<InputsManager*>(glfwGetWindowUserPointer(window));
							  if (self)
								  self->MouseScrollCallback(xoffset, yoffset);
						  });
}

void InputsManager::FramebufferSizeCallback(int width, int height)
{
	std::unique_lock<std::mutex> lock(m_MutexFramebuffer);
	m_FramebufferState.Resized = true;
	m_FramebufferState.Width = width;
	m_FramebufferState.Height = height;
}

void InputsManager::MouseScrollCallback(double xoffset, double yoffset)
{
	std::unique_lock<std::mutex> lock(m_MutexMouse);
	m_MouseState.ScrollOffsetChanged = true;
	m_MouseState.ScrollXoffset = xoffset;
	m_MouseState.ScrollYoffset = yoffset;
}

void InputsManager::SetMouseCaptureEnabled(bool enabled)
{
	bool wasEnabled = m_MouseCaptureEnabled;
	m_MouseCaptureEnabled = enabled;
	if (enabled)
	{
		glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Capture mouse

		if (!wasEnabled)
		{
			m_FirstMouse = true; // Reset first mouse flag when enabling capture (Avoid sudden jump)
		}
	}
	else if (!enabled && wasEnabled)
	{
		glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // Release mouse
	}
}

bool InputsManager::IsMouseCaptureEnabled() const
{
	return m_MouseCaptureEnabled;
}

FramebufferState InputsManager::GetFramebufferState()
{
	std::unique_lock<std::mutex> lock(m_MutexFramebuffer);
	return m_FramebufferState;
}

int InputsManager::RegisterInput(const Key key, InputConfig config)
{
	std::unique_lock<std::mutex> lock(m_MutexRegisteredInputs);

	int inputId = m_NextInputId++;

	KeyInputControl keyControl;
	keyControl.key = key;
	keyControl.EnableControlledKeyRepeat = config.EnableKeyRepeat;
	keyControl.KeyRepeatDelay = config.KeyRepeatDelay;
	keyControl.KeyRepeatInterval = config.KeyRepeatInterval;
	keyControl.DoublePressDelay = config.DoublePressDelay;

	m_RegisteredInputs[inputId] = keyControl;

	return inputId;
}

void InputsManager::UnregisterInput(int inputId)
{
	std::unique_lock<std::mutex> lock(m_MutexRegisteredInputs);
	m_RegisteredInputs.erase(inputId);
}

void InputsManager::UpdateInputsSnapshot()
{
	std::shared_ptr<InputsSnapshot> inputs = std::make_shared<InputsSnapshot>();

	{
		std::unique_lock lock(m_MutexFramebuffer);
		inputs->Framebuffer = m_FramebufferState;
		m_FramebufferState.Resized = false; // Reset the framebuffer resized flag
	}

	{
		std::unique_lock<std::mutex> lock(m_MutexMouse);
		inputs->Mouse = m_MouseState;
	}

	{
		std::unique_lock<std::mutex> lockInputs(m_MutexRegisteredInputs);
		for (const auto& [inputId, keyControl] : m_RegisteredInputs)
		{
			KeyState keyState;
			keyState.IsPressed = keyControl.IsPressed();
			keyState.IsDoublePressed = keyControl.IsDoublePressed();
			inputs->KeysStates[inputId] = keyState;
		}
	}

	std::unique_lock<std::mutex> lockInputs(m_MutexSnapshot);
	m_InputsSnapshot = inputs;
}

void InputsManager::KeyInputControl::Update(bool isKeyDown)
{
	double now = glfwGetTime();

	if (EnableControlledKeyRepeat == false)
	{
		m_IsPressed = isKeyDown;
		return;
	}

	// Handle Repeated Key presses
	if (isKeyDown)
	{
		if (!m_WasDown)
		{
			// Key was just pressed this frame
			m_IsPressed = true; // This frame, report as pressed
			m_IsHeld = true;
			m_FirstDelay = true;
			m_LastPressedTime = now;
		}
		else if (m_IsHeld)
		{
			if (m_FirstDelay)
			{
				if (now - m_LastPressedTime >= KeyRepeatDelay)
				{
					m_IsPressed = true; // First repeat
					m_FirstDelay = false;
					m_LastPressedTime = now;
				}
				else
				{
					m_IsPressed = false; // Wait for initial delay
				}
			}
			else
			{
				if (now - m_LastPressedTime >= KeyRepeatInterval)
				{
					m_IsPressed = true; // Subsequent repeats
					m_LastPressedTime = now;
				}
				else
				{
					m_IsPressed = false; // Not yet time for another repeat
				}
			}
		}
	}
	else
	{
		// Key is up
		m_IsPressed = false;
		m_IsHeld = false;
		m_FirstDelay = true;
		m_LastPressedTime = 0.0;
	}

	// Handle Double Key presses
	m_IsDoublePressed = false; // Reset double pressed state
	if (isKeyDown && !m_WasDown)
	{
		// Key was just pressed this frame
		// Check for double press
		if (m_LastPressedTimeDouble != 0.0 && (now - m_LastPressedTimeDouble <= DoublePressDelay))
		{
			m_IsDoublePressed = true;	   // Double press detected
			m_LastPressedTimeDouble = 0.0; // Reset for next detection
		}
		else
		{
			m_LastPressedTimeDouble = now; // Update last pressed time
		}
	}

	m_WasDown = isKeyDown;
}

bool InputsManager::KeyInputControl::IsPressed() const
{
	return m_IsPressed;
}

bool InputsManager::KeyInputControl::IsDoublePressed() const
{
	return m_IsDoublePressed;
}

KeyState InputsSnapshot::GetKeyState(int inputId) const
{
	auto it = KeysStates.find(inputId);
	if (it != KeysStates.end())
	{
		return it->second;
	}
	throw std::runtime_error("Input ID not found in InputsSnapshot Key States.");
}
