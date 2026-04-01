#include <GLFW/glfw3.h>

#include <unordered_map>

namespace onion::voxel
{
	enum class Key : int
	{
		Unknown = GLFW_KEY_UNKNOWN,

		MouseButtonLeft = GLFW_MOUSE_BUTTON_LEFT,
		MouseButtonRight = GLFW_MOUSE_BUTTON_RIGHT,
		MouseButtonMiddle = GLFW_MOUSE_BUTTON_MIDDLE,
		MouseButton4 = GLFW_MOUSE_BUTTON_4,
		MouseButton5 = GLFW_MOUSE_BUTTON_5,
		MouseButton6 = GLFW_MOUSE_BUTTON_6,
		MouseButton7 = GLFW_MOUSE_BUTTON_7,
		MouseButton8 = GLFW_MOUSE_BUTTON_8,

		// Printable keys

		Space = GLFW_KEY_SPACE,
		Apostrophe = GLFW_KEY_APOSTROPHE, // '
		Comma = GLFW_KEY_COMMA,			  // ,
		Minus = GLFW_KEY_MINUS,			  // -
		Period = GLFW_KEY_PERIOD,		  // .
		Slash = GLFW_KEY_SLASH,			  // /
		Num0 = GLFW_KEY_0,
		Num1 = GLFW_KEY_1,
		Num2 = GLFW_KEY_2,
		Num3 = GLFW_KEY_3,
		Num4 = GLFW_KEY_4,
		Num5 = GLFW_KEY_5,
		Num6 = GLFW_KEY_6,
		Num7 = GLFW_KEY_7,
		Num8 = GLFW_KEY_8,
		Num9 = GLFW_KEY_9,
		Semicolon = GLFW_KEY_SEMICOLON, // ;
		Equal = GLFW_KEY_EQUAL,			// =
		A = GLFW_KEY_A,
		B = GLFW_KEY_B,
		C = GLFW_KEY_C,
		D = GLFW_KEY_D,
		E = GLFW_KEY_E,
		F = GLFW_KEY_F,
		G = GLFW_KEY_G,
		H = GLFW_KEY_H,
		I = GLFW_KEY_I,
		J = GLFW_KEY_J,
		K = GLFW_KEY_K,
		L = GLFW_KEY_L,
		M = GLFW_KEY_M,
		N = GLFW_KEY_N,
		O = GLFW_KEY_O,
		P = GLFW_KEY_P,
		Q = GLFW_KEY_Q,
		R = GLFW_KEY_R,
		S = GLFW_KEY_S,
		T = GLFW_KEY_T,
		U = GLFW_KEY_U,
		V = GLFW_KEY_V,
		W = GLFW_KEY_W,
		X = GLFW_KEY_X,
		Y = GLFW_KEY_Y,
		Z = GLFW_KEY_Z,
		LeftBracket = GLFW_KEY_LEFT_BRACKET, // [
		Backslash = GLFW_KEY_BACKSLASH,
		RightBracket = GLFW_KEY_RIGHT_BRACKET, // ]
		GraveAccent = GLFW_KEY_GRAVE_ACCENT,   // `
		World1 = GLFW_KEY_WORLD_1,			   // non-US #1
		World2 = GLFW_KEY_WORLD_2,			   // non-US #2

		// Function keys

		Escape = GLFW_KEY_ESCAPE,
		Enter = GLFW_KEY_ENTER,
		Tab = GLFW_KEY_TAB,
		Backspace = GLFW_KEY_BACKSPACE,
		Insert = GLFW_KEY_INSERT,
		Delete = GLFW_KEY_DELETE,
		Right = GLFW_KEY_RIGHT,
		Left = GLFW_KEY_LEFT,
		Down = GLFW_KEY_DOWN,
		Up = GLFW_KEY_UP,
		PageUp = GLFW_KEY_PAGE_UP,
		PageDown = GLFW_KEY_PAGE_DOWN,
		Home = GLFW_KEY_HOME,
		End = GLFW_KEY_END,
		CapsLock = GLFW_KEY_CAPS_LOCK,
		ScrollLock = GLFW_KEY_SCROLL_LOCK,
		NumLock = GLFW_KEY_NUM_LOCK,
		PrintScreen = GLFW_KEY_PRINT_SCREEN,
		Pause = GLFW_KEY_PAUSE,
		F1 = GLFW_KEY_F1,
		F2 = GLFW_KEY_F2,
		F3 = GLFW_KEY_F3,
		F4 = GLFW_KEY_F4,
		F5 = GLFW_KEY_F5,
		F6 = GLFW_KEY_F6,
		F7 = GLFW_KEY_F7,
		F8 = GLFW_KEY_F8,
		F9 = GLFW_KEY_F9,
		F10 = GLFW_KEY_F10,
		F11 = GLFW_KEY_F11,
		F12 = GLFW_KEY_F12,
		F13 = GLFW_KEY_F13,
		F14 = GLFW_KEY_F14,
		F15 = GLFW_KEY_F15,
		F16 = GLFW_KEY_F16,
		F17 = GLFW_KEY_F17,
		F18 = GLFW_KEY_F18,
		F19 = GLFW_KEY_F19,
		F20 = GLFW_KEY_F20,
		F21 = GLFW_KEY_F21,
		F22 = GLFW_KEY_F22,
		F23 = GLFW_KEY_F23,
		F24 = GLFW_KEY_F24,
		F25 = GLFW_KEY_F25,

		// Keypad

		KP0 = GLFW_KEY_KP_0,
		KP1 = GLFW_KEY_KP_1,
		KP2 = GLFW_KEY_KP_2,
		KP3 = GLFW_KEY_KP_3,
		KP4 = GLFW_KEY_KP_4,
		KP5 = GLFW_KEY_KP_5,
		KP6 = GLFW_KEY_KP_6,
		KP7 = GLFW_KEY_KP_7,
		KP8 = GLFW_KEY_KP_8,
		KP9 = GLFW_KEY_KP_9,
		KPDecimal = GLFW_KEY_KP_DECIMAL,
		KPDivide = GLFW_KEY_KP_DIVIDE,
		KPMultiply = GLFW_KEY_KP_MULTIPLY,
		KPSubtract = GLFW_KEY_KP_SUBTRACT,
		KPAdd = GLFW_KEY_KP_ADD,
		KPEnter = GLFW_KEY_KP_ENTER,
		KPEqual = GLFW_KEY_KP_EQUAL,

		// Modifiers & system keys

		LeftShift = GLFW_KEY_LEFT_SHIFT,
		LeftControl = GLFW_KEY_LEFT_CONTROL,
		LeftAlt = GLFW_KEY_LEFT_ALT,
		LeftSuper = GLFW_KEY_LEFT_SUPER, // Windows / Command key
		RightShift = GLFW_KEY_RIGHT_SHIFT,
		RightControl = GLFW_KEY_RIGHT_CONTROL,
		RightAlt = GLFW_KEY_RIGHT_ALT,
		RightSuper = GLFW_KEY_RIGHT_SUPER,
		Menu = GLFW_KEY_MENU
	};

	enum class CursorStyle : int
	{
		Arrow = GLFW_ARROW_CURSOR,
		IBeam = GLFW_IBEAM_CURSOR,
		Crosshair = GLFW_CROSSHAIR_CURSOR,
		Hand = GLFW_HAND_CURSOR,
		HResize = GLFW_HRESIZE_CURSOR,
		VResize = GLFW_VRESIZE_CURSOR
	};

	inline std::unordered_map<Key, std::string> KeyToStringMap = {
		{Key::Unknown, "Unknown"},

		// Mouse
		{Key::MouseButtonLeft, "Mouse Left"},
		{Key::MouseButtonRight, "Mouse Right"},
		{Key::MouseButtonMiddle, "Mouse Middle"},
		{Key::MouseButton4, "Mouse Button 4"},
		{Key::MouseButton5, "Mouse Button 5"},
		{Key::MouseButton6, "Mouse Button 6"},
		{Key::MouseButton7, "Mouse Button 7"},
		{Key::MouseButton8, "Mouse Button 8"},

		// Printable
		{Key::Space, "Space"},
		{Key::Apostrophe, "'"},
		{Key::Comma, ","},
		{Key::Minus, "-"},
		{Key::Period, "."},
		{Key::Slash, "/"},
		{Key::Num0, "0"},
		{Key::Num1, "1"},
		{Key::Num2, "2"},
		{Key::Num3, "3"},
		{Key::Num4, "4"},
		{Key::Num5, "5"},
		{Key::Num6, "6"},
		{Key::Num7, "7"},
		{Key::Num8, "8"},
		{Key::Num9, "9"},
		{Key::Semicolon, ";"},
		{Key::Equal, "="},

		{Key::A, "A"},
		{Key::B, "B"},
		{Key::C, "C"},
		{Key::D, "D"},
		{Key::E, "E"},
		{Key::F, "F"},
		{Key::G, "G"},
		{Key::H, "H"},
		{Key::I, "I"},
		{Key::J, "J"},
		{Key::K, "K"},
		{Key::L, "L"},
		{Key::M, "M"},
		{Key::N, "N"},
		{Key::O, "O"},
		{Key::P, "P"},
		{Key::Q, "Q"},
		{Key::R, "R"},
		{Key::S, "S"},
		{Key::T, "T"},
		{Key::U, "U"},
		{Key::V, "V"},
		{Key::W, "W"},
		{Key::X, "X"},
		{Key::Y, "Y"},
		{Key::Z, "Z"},

		{Key::LeftBracket, "["},
		{Key::Backslash, "\\"},
		{Key::RightBracket, "]"},
		{Key::GraveAccent, "`"},
		{Key::World1, "World 1"},
		{Key::World2, "World 2"},

		// Function / control
		{Key::Escape, "Escape"},
		{Key::Enter, "Enter"},
		{Key::Tab, "Tab"},
		{Key::Backspace, "Backspace"},
		{Key::Insert, "Insert"},
		{Key::Delete, "Delete"},

		{Key::Right, "Right Arrow"},
		{Key::Left, "Left Arrow"},
		{Key::Down, "Down Arrow"},
		{Key::Up, "Up Arrow"},

		{Key::PageUp, "Page Up"},
		{Key::PageDown, "Page Down"},
		{Key::Home, "Home"},
		{Key::End, "End"},

		{Key::CapsLock, "Caps Lock"},
		{Key::ScrollLock, "Scroll Lock"},
		{Key::NumLock, "Num Lock"},
		{Key::PrintScreen, "Print Screen"},
		{Key::Pause, "Pause"},

		{Key::F1, "F1"},
		{Key::F2, "F2"},
		{Key::F3, "F3"},
		{Key::F4, "F4"},
		{Key::F5, "F5"},
		{Key::F6, "F6"},
		{Key::F7, "F7"},
		{Key::F8, "F8"},
		{Key::F9, "F9"},
		{Key::F10, "F10"},
		{Key::F11, "F11"},
		{Key::F12, "F12"},
		{Key::F13, "F13"},
		{Key::F14, "F14"},
		{Key::F15, "F15"},
		{Key::F16, "F16"},
		{Key::F17, "F17"},
		{Key::F18, "F18"},
		{Key::F19, "F19"},
		{Key::F20, "F20"},
		{Key::F21, "F21"},
		{Key::F22, "F22"},
		{Key::F23, "F23"},
		{Key::F24, "F24"},
		{Key::F25, "F25"},

		// Keypad
		{Key::KP0, "Keypad 0"},
		{Key::KP1, "Keypad 1"},
		{Key::KP2, "Keypad 2"},
		{Key::KP3, "Keypad 3"},
		{Key::KP4, "Keypad 4"},
		{Key::KP5, "Keypad 5"},
		{Key::KP6, "Keypad 6"},
		{Key::KP7, "Keypad 7"},
		{Key::KP8, "Keypad 8"},
		{Key::KP9, "Keypad 9"},
		{Key::KPDecimal, "Keypad ."},
		{Key::KPDivide, "Keypad /"},
		{Key::KPMultiply, "Keypad *"},
		{Key::KPSubtract, "Keypad -"},
		{Key::KPAdd, "Keypad +"},
		{Key::KPEnter, "Keypad Enter"},
		{Key::KPEqual, "Keypad ="},

		// Modifiers
		{Key::LeftShift, "Left Shift"},
		{Key::LeftControl, "Left Control"},
		{Key::LeftAlt, "Left Alt"},
		{Key::LeftSuper, "Left Super"},
		{Key::RightShift, "Right Shift"},
		{Key::RightControl, "Right Control"},
		{Key::RightAlt, "Right Alt"},
		{Key::RightSuper, "Right Super"},
		{Key::Menu, "Menu"},
	};

	inline std::unordered_map<std::string, Key> StringToKeyMap = []()
	{
		std::unordered_map<std::string, Key> map;
		for (const auto& [key, str] : KeyToStringMap)
		{
			map[str] = key;
		}
		return map;
	}();

	inline std::string KeyToString(Key key)
	{
		auto it = KeyToStringMap.find(key);
		if (it != KeyToStringMap.end())
			return it->second;

		throw std::runtime_error("Invalid key enum value");
	}

	inline Key StringToKey(const std::string& str)
	{
		auto it = StringToKeyMap.find(str);
		if (it != StringToKeyMap.end())
			return it->second;

		throw std::runtime_error("Invalid key string: " + str);
	}

} // namespace onion::voxel
