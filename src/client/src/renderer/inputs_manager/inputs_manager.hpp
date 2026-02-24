#pragma once

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <shared_mutex>
#include <unordered_map>

#include "inputs.hpp"

namespace onion::voxel
{
	/// @brief Represents the state of the framebuffer, including its size and whether it has been resized since the last snapshot.
	struct FramebufferState
	{
		/// @brief Indicates whether the framebuffer has been resized since the last snapshot.
		bool Resized = false;
		/// @brief The current width of the framebuffer in pixels.
		int Width = 800;
		/// @brief The current height of the framebuffer in pixels.
		int Height = 600;
	};

	/// @brief Represents the state of the mouse, including its position, movement offsets, scroll offsets, and button states.
	struct MouseState
	{
		/// @brief Indicates whether mouse capture is enabled. When enabled, the mouse cursor is hidden and its movement is captured for relative motion, typically used in first-person camera controls.
		bool CaptureEnabled = true;

		/// @brief Indicates whether the mouse has moved since the last snapshot.
		bool MovementOffsetChanged = false;
		/// @brief The offset of the mouse movement in the X direction since the last snapshot.
		double Xoffset = 0.f;
		/// @brief The offset of the mouse movement in the Y direction since the last snapshot.
		double Yoffset = 0.f;

		/// @brief The current X position of the mouse cursor in screen coordinates.
		double Xpos = 0.f;
		/// @brief The current Y position of the mouse cursor in screen coordinates.
		double Ypos = 0.f;

		/// @brief Indicates whether the scroll offset has changed since the last snapshot.
		bool ScrollOffsetChanged = false;
		/// @brief The offset of the scroll in the X direction since the last snapshot.
		double ScrollXoffset = 0.f;
		/// @brief The offset of the scroll in the Y direction since the last snapshot.
		double ScrollYoffset = 0.f;

		/// @brief Indicates whether the left mouse button is currently pressed.
		bool LeftButtonPressed = false;
		/// @brief Indicates whether the right mouse button is currently pressed.
		bool RightButtonPressed = false;
	};

	/// @brief Represents the state of a keyboard key, including whether it is currently pressed and whether it has been double-pressed since the last snapshot.
	struct KeyState
	{
		/// @brief Indicates whether the key is currently pressed.
		bool IsPressed = false;
		/// @brief Indicates whether the key has been double-pressed since the last snapshot.
		bool IsDoublePressed = false;
	};

	/// @brief Represents a snapshot of the current input states, including the framebuffer state, mouse state, and keyboard key states. This snapshot is used to provide a consistent view of the inputs for each frame.
	struct InputsSnapshot
	{
		/// @brief The state of the framebuffer at the time of the snapshot.
		FramebufferState Framebuffer;
		/// @brief The state of the mouse at the time of the snapshot.
		MouseState Mouse;
		/// @brief A mapping of input IDs to their corresponding key states at the time of the snapshot.
		std::unordered_map<int, KeyState> KeysStates;
		/// @brief Retrieves the state of a specific key by its input ID.
		/// @param inputId The ID of the input to retrieve the state for.
		/// @return The state of the specified key.
		KeyState GetKeyState(int inputId) const;
	};

	/// @brief Configuration for a specific input, including settings for key repeat behavior and double-press detection.
	struct InputConfig
	{
		/// @brief Indicates whether controlled key repeat is enabled when holding the key. If enabled, the key will repeat after a delay when held down.
		bool EnableKeyRepeat = false;
		/// @brief The delay in seconds before the key starts repeating when held down. This is only relevant if EnableKeyRepeat is true.
		float KeyRepeatDelay = 0.6f;
		/// @brief The interval in seconds between repeated key presses when the key is held down. This is only relevant if EnableKeyRepeat is true.
		float KeyRepeatInterval = 0.4f;
		/// @brief The delay in seconds between consecutive key presses for detecting a double key press.
		float DoublePressDelay = 0.5f;
	};

	/// @brief Manages the input states for the application, including keyboard and mouse inputs. It provides functionality to register custom inputs, capture mouse movement, and take snapshots of the current input states for use in the rendering loop.
	class InputsManager
	{
	  public:
		InputsManager() = default;
		~InputsManager() = default;

		/// @brief Initializes the InputsManager with the given GLFW window.
		/// @param window The GLFW window to associate with the InputsManager for capturing inputs.
		/// @details  This sets up the necessary callbacks and prepares the manager to capture inputs from the specified window. It must be called before using any other functionality of the InputsManager.
		void Init(GLFWwindow* window);

		/// @brief Polls the current input states and updates the internal snapshot. This should be called once per frame to ensure that the input states are up-to-date for the rendering loop.
		void PoolInputs();
		/// @brief Retrieves a snapshot of the current input states, including framebuffer state, mouse state, and keyboard key states. This snapshot is intended to provide a consistent view of the inputs for the current frame and should be called after PoolInputs() to ensure it reflects the latest input states.
		/// @return A shared pointer to an InputsSnapshot containing the current input states.
		/// @throws std::runtime_error if the InputsSnapshot is not initialized, which can occur if PoolInputs() has not been called before this method.
		std::shared_ptr<InputsSnapshot> GetInputsSnapshot();

		/// @brief Enables or disables mouse capture. When enabled, the mouse cursor is hidden and its movement is captured for relative motion, which is typically used in first-person camera controls. When disabled, the mouse cursor is visible and its position is not captured for relative motion.
		void SetMouseCaptureEnabled(bool enabled);
		/// @brief Checks whether mouse capture is currently enabled. When mouse capture is enabled, the mouse cursor is hidden and its movement is captured for relative motion.
		bool IsMouseCaptureEnabled() const;

		/// @brief Retrieves the current state of the framebuffer, including its width and height.
		/// @return A FramebufferState object containing the current framebuffer dimensions.
		FramebufferState GetFramebufferState();

		/// @brief Registers a new input with the specified key and configuration. This allows the application to track the state of specific keys according to the provided configuration for key repeat and double-press detection.
		/// @param key The key to register for input tracking.
		/// @param config The configuration for the input, including settings for key repeat and double-press detection. If not provided, default settings will be used.
		/// @return An integer ID that can be used to reference this input when retrieving its state from the InputsSnapshot.
		/// @details This method adds a new input to the manager's tracking system. The returned input ID can be used in the InputsSnapshot to check the state of this specific input during the rendering loop. It is important to manage these IDs properly, especially if inputs are unregistered, to avoid referencing invalid IDs.
		int RegisterInput(const Key key, InputConfig config = InputConfig());

		/// @brief Unregisters an input by its ID, removing it from the manager's tracking system. After an input is unregistered, its state will no longer be available in the InputsSnapshot, and any references to its ID should be removed to avoid errors.
		void UnregisterInput(int inputId);

		// ------------ INPUTS SNAPSHOT MANAGEMENT ------------
	  private:
		mutable std::mutex m_MutexSnapshot;
		std::shared_ptr<InputsSnapshot> m_InputsSnapshot;
		void UpdateInputsSnapshot();

		// ------------ GLFW & Time Management ------------
	  private:
		GLFWwindow* m_Window = nullptr;
		double m_GlfwTime = 0.f;

		// ------------ Registered Inputs Management ------------
	  private:
		class KeyInputControl;
		mutable std::mutex m_MutexRegisteredInputs;
		int m_NextInputId = 1;
		std::unordered_map<int, KeyInputControl> m_RegisteredInputs;

		// ------------ Internal States ------------
	  private:
		mutable std::mutex m_MutexMouse;
		MouseState m_MouseState;
		bool m_MouseCaptureEnabled = true;
		bool m_FirstMouse = true;
		double m_MouseLastX = 0.f;
		double m_MouseLastY = 0.f;
		double m_MouseXoffset = 0.f;
		double m_MouseYoffset = 0.f;

		mutable std::mutex m_MutexFramebuffer;
		FramebufferState m_FramebufferState;

		// ------------ Internal Polling Methods ------------
	  private:
		void PoolMouseMovement();
		void PoolMouseInputs();
		void PollKeyboardInputs();

		// ------------ Internal State Management Methods ------------
	  private:
		void ResetFlags();

		// ------------ Callbacks ------------
	  private:
		void InitCallbacks();
		void FramebufferSizeCallback(int width, int height);
		void MouseScrollCallback(double xoffset, double yoffset);

		// ------------ Deleted Copy & Move Constructors and Assignment Operators ------------
	  public:
		InputsManager(const InputsManager&) = delete;
		InputsManager& operator=(const InputsManager&) = delete;
		InputsManager(InputsManager&&) = delete;
		InputsManager& operator=(InputsManager&&) = delete;

		// ------------ Key Input Control Class ------------
	  private:
		class KeyInputControl
		{
		  public:
			KeyInputControl() = default;
			~KeyInputControl() = default;

			void Update(bool isKeyDown);

			bool IsPressed() const;
			bool IsDoublePressed() const;

			Key key = Key::Unknown;

			bool EnableControlledKeyRepeat = false;
			double KeyRepeatDelay = 0.6f;	 // Delay before repeating the key press
			double KeyRepeatInterval = 0.4f; // Interval between repeated key presses

			double DoublePressDelay = 0.5f; // Delay for double key press detection
		  private:
			// Previous state
			bool m_WasDown = false;

			// For Repeated Key Presses
			bool m_IsPressed = false;
			bool m_IsHeld = false;
			bool m_FirstDelay = true;
			double m_LastPressedTime = 0.0;

			// For Double Key Presses
			bool m_IsDoublePressed = false;
			double m_LastPressedTimeDouble = 0.0;
		};
	};

} // namespace onion::voxel
