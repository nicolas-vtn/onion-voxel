#pragma once

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <shared_mutex>
#include <unordered_map>

#include "inputs.hpp"

namespace onion::voxel
{

	struct FramebufferState
	{
		bool Resized = false;
		int Width = 800;
		int Height = 600;
	};

	struct MouseState
	{
		bool CaptureEnabled = true;

		bool MovementOffsetChanged = false;
		double Xoffset = 0.f;
		double Yoffset = 0.f;

		double Xpos = 0.f;
		double Ypos = 0.f;

		bool ScrollOffsetChanged = false;
		double ScrollXoffset = 0.f;
		double ScrollYoffset = 0.f;

		bool LeftButtonPressed = false;
		bool RightButtonPressed = false;
	};

	struct KeyState
	{
		bool IsPressed = false;
		bool IsDoublePressed = false;
	};

	struct InputsSnapshot
	{
		FramebufferState Framebuffer;
		MouseState Mouse;
		std::unordered_map<int, KeyState> KeysStates;
		KeyState GetKeyState(int inputId) const;
	};

	struct InputConfig
	{

		InputConfig(bool enableControlledKeyRepeatWhenHold = false,
					double keyRepeatDelayWhenHold = 0.6f,
					double keyRepeatIntervalWhenHold = 0.4f,
					double doublePressDelay = 0.5f)
			: EnableControlledKeyRepeatWhenHold(enableControlledKeyRepeatWhenHold),
			  KeyRepeatDelayWhenHold(keyRepeatDelayWhenHold), KeyRepeatIntervalWhenHold(keyRepeatIntervalWhenHold),
			  DoublePressDelay(doublePressDelay)
		{
		}

		bool EnableControlledKeyRepeatWhenHold; // Enable controlled key repeat when holding the key
		double KeyRepeatDelayWhenHold;			// Delay before repeating the key press
		double KeyRepeatIntervalWhenHold;		// Interval between repeated key presses
		double DoublePressDelay;				// Delay for double key press detection
	};

	class InputsManager
	{
	  public:
		InputsManager() = default;
		~InputsManager() = default;

		void Init(GLFWwindow* window);

		void PoolInputs();
		std::shared_ptr<InputsSnapshot> GetInputsSnapshot();

		void SetMouseCaptureEnabled(bool enabled);
		bool IsMouseCaptureEnabled() const;

		void GetFramebufferSize(int& width, int& height);

		int RegisterInput(const Key key, InputConfig config = InputConfig());
		void UnregisterInput(int inputId);

	  private:
		mutable std::mutex m_MutexSnapshot;
		std::shared_ptr<InputsSnapshot> m_InputsSnapshot;
		void UpdateInputsSnapshot();

	  private:
		class KeyInputControl;

	  private:
		GLFWwindow* m_Window = nullptr;
		double m_GlfwTime = 0.f;

	  private:
		mutable std::mutex m_MutexRegisteredInputs;
		int m_NextInputId = 1;
		std::unordered_map<int, KeyInputControl> m_RegisteredInputs;

		// Internal States
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

		// Pool inputs
	  private:
		void PoolMouseMovement();
		void PoolMouseInputs();
		void PollKeyboardInputs();

	  private:
		void ResetFlags();

	  private:
		// Callbacks
	  private:
		void InitCallbacks();
		void FramebufferSizeCallback(int width, int height);
		void MouseScrollCallback(double xoffset, double yoffset);

	  public:
		InputsManager(const InputsManager&) = delete;
		InputsManager& operator=(const InputsManager&) = delete;
		InputsManager(InputsManager&&) = delete;
		InputsManager& operator=(InputsManager&&) = delete;

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
