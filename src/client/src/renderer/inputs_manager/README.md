# InputsManager (C++ / GLFW)

`InputsManager` is a small, GLFW-based input layer that turns raw keyboard + mouse state into a **stable per-frame snapshot** you can consume in your game loop.

It supports:
- **Keyboard**: pressed state, optional **controlled key-repeat**, optional **double-press** detection
- **Mouse**: cursor position, **relative movement offsets**, scroll offsets, left/right buttons
- **Framebuffer**: current size + a “resized since last frame” flag
- **Snapshot model**: read inputs from an `InputsSnapshot` that represents **one frame** (avoids querying GLFW everywhere)

---

## Dependencies

### External
- **GLFW** (input + window callbacks)
- **glad** (included by the header; not used directly by `InputsManager` logic, but required by includes)

### Standard library
- `<mutex>`, `<shared_mutex>` (sync)
- `<unordered_map>`
- `<memory>` (for `std::shared_ptr`)

### Internal
- `inputs.hpp` (defines `onion::voxel::Key` mapping to GLFW key codes)

---

## Concept: register inputs → poll each frame → read snapshot

`InputsManager` is built around 3 steps:

1. **Init** with a `GLFWwindow*` (sets callbacks and prepares internal state)
2. **Register** the keys you care about (returns an `inputId`)
3. Every frame:
   - call `PoolInputs()`
   - call `GetInputsSnapshot()` and query states from that snapshot

---

## Public API overview

### Initialization
- `void Init(GLFWwindow* window);`

Must be called once before use.

### Per-frame polling
- `void PoolInputs();`
- `std::shared_ptr<InputsSnapshot> GetInputsSnapshot();`

Call `PoolInputs()` once per frame, then read the snapshot.

> `GetInputsSnapshot()` throws `std::runtime_error` if the snapshot is not initialized (typically if you forgot to call `PoolInputs()` at least once).

### Mouse capture
- `void SetMouseCaptureEnabled(bool enabled);`
- `bool IsMouseCaptureEnabled() const;`

When capture is enabled, the cursor is hidden and mouse movement is tracked as **relative offsets** (useful for FPS camera).

### Framebuffer state
- `FramebufferState GetFramebufferState();`

Retrieve latest framebuffer dimensions and resize flag.

### Custom inputs (keyboard)
- `int RegisterInput(const Key key, InputConfig config = InputConfig());`
- `void UnregisterInput(int inputId);`

Register keys once (at startup, or when bindings change). You’ll use the returned `inputId` to query state from snapshots.

---

## Data structures you will read from

### `InputsSnapshot`
Contains a per-frame view:
- `FramebufferState Framebuffer;`
- `MouseState Mouse;`
- `std::unordered_map<int, KeyState> KeysStates;`
- `KeyState GetKeyState(int inputId) const;`

### `KeyState`
- `bool IsPressed;` — true when the input is considered “pressed” for this frame
- `bool IsDoublePressed;` — true when a double press was detected (based on config)

### `MouseState`
- `bool CaptureEnabled;`
- `bool MovementOffsetChanged;`
- `double Xoffset, Yoffset;` — mouse delta since last snapshot (relative motion)
- `double Xpos, Ypos;` — absolute cursor position
- `bool ScrollOffsetChanged;`
- `double ScrollXoffset, ScrollYoffset;`
- `bool LeftButtonPressed, RightButtonPressed;`

### `FramebufferState`
- `bool Resized;`
- `int Width, Height;`

---

## Demo: minimal integration (typical game loop)

This example shows:
- Registering inputs (`W`, `A`, `S`, `D`, `Escape`)
- Polling each frame
- Reading keyboard + mouse + framebuffer state from the snapshot
- Toggling mouse capture with `Escape` (example behavior)

```cpp
#include <iostream>
#include <memory>

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "InputsManager.hpp" // your header path
// includes inputs.hpp internally (Key enum)

using namespace onion::voxel;

int main()
{
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    GLFWwindow* window = glfwCreateWindow(800, 600, "InputsManager Demo", nullptr, nullptr);
    if (!window)
        return -1;

    glfwMakeContextCurrent(window);
    gladLoadGL();

    InputsManager inputs;
    inputs.Init(window);

    // Register inputs you want to track
    const int moveForward = inputs.RegisterInput(Key::W);
    const int moveLeft    = inputs.RegisterInput(Key::A);
    const int moveBack    = inputs.RegisterInput(Key::S);
    const int moveRight   = inputs.RegisterInput(Key::D);

    // Example: Escape with double-press detection
    InputConfig escCfg;
    escCfg.DoublePressDelay = 0.35f;
    const int esc = inputs.RegisterInput(Key::Escape, escCfg);

    while (!glfwWindowShouldClose(window))
    {
        // Typical GLFW pump
        glfwPollEvents();

        // 1) Update internal input states
        inputs.PoolInputs();

        // 2) Read a stable snapshot for this frame
        auto snap = inputs.GetInputsSnapshot();

        // --- Framebuffer resize handling
        if (snap->Framebuffer.Resized)
        {
            glViewport(0, 0, snap->Framebuffer.Width, snap->Framebuffer.Height);
            std::cout << "Resized: " << snap->Framebuffer.Width
                      << "x" << snap->Framebuffer.Height << "\n";
        }

        // --- Keyboard
        const bool w = snap->GetKeyState(moveForward).IsPressed;
        const bool a = snap->GetKeyState(moveLeft).IsPressed;
        const bool s = snap->GetKeyState(moveBack).IsPressed;
        const bool d = snap->GetKeyState(moveRight).IsPressed;

        if (w || a || s || d)
        {
            std::cout << "Move: "
                      << (w ? "W" : "") << (a ? "A" : "")
                      << (s ? "S" : "") << (d ? "D" : "") << "\n";
        }

        // Toggle mouse capture on Escape double-press (example behavior)
        if (snap->GetKeyState(esc).IsDoublePressed)
        {
            const bool enabled = !inputs.IsMouseCaptureEnabled();
            inputs.SetMouseCaptureEnabled(enabled);
            std::cout << "Mouse capture: " << (enabled ? "ON" : "OFF") << "\n";
        }

        // --- Mouse (relative movement)
        if (snap->Mouse.MovementOffsetChanged && snap->Mouse.CaptureEnabled)
        {
            const double dx = snap->Mouse.Xoffset;
            const double dy = snap->Mouse.Yoffset;

            // Example: feed your camera yaw/pitch
            // cameraYaw   += dx * sensitivity;
            // cameraPitch += dy * sensitivity;

            std::cout << "Mouse delta: " << dx << ", " << dy << "\n";
        }

        // --- Mouse scroll
        if (snap->Mouse.ScrollOffsetChanged)
        {
            std::cout << "Scroll: " << snap->Mouse.ScrollXoffset
                      << ", " << snap->Mouse.ScrollYoffset << "\n";
        }

        // --- Mouse buttons
        if (snap->Mouse.LeftButtonPressed)
            std::cout << "LMB down\n";
        if (snap->Mouse.RightButtonPressed)
            std::cout << "RMB down\n";

        // Render...
        glClear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
````

---

## Demo: controlled key-repeat (for menus / UI navigation)

If you want “press once, then repeat at a fixed rate while held” (instead of relying on OS key-repeat),
enable controlled repeat in `InputConfig`.

```cpp
InputConfig navCfg;
navCfg.EnableKeyRepeat   = true;
navCfg.KeyRepeatDelay    = 0.35f; // seconds before repeating
navCfg.KeyRepeatInterval = 0.08f; // seconds between repeats

int up    = inputs.RegisterInput(Key::Up, navCfg);
int down  = inputs.RegisterInput(Key::Down, navCfg);
int enter = inputs.RegisterInput(Key::Enter); // normal press
```

Then per frame:

```cpp
auto snap = inputs.GetInputsSnapshot();

if (snap->GetKeyState(up).IsPressed)   { /* move selection up */ }
if (snap->GetKeyState(down).IsPressed) { /* move selection down */ }
if (snap->GetKeyState(enter).IsPressed){ /* activate */ }
```

With controlled repeat enabled, `IsPressed` will “pulse” repeatedly while the key is held, according to your delay/interval.

---

## Notes & best practices

* **Register once, query by ID forever**: store `inputId`s in your player/controller/UI code.
* **Call order matters**: `PoolInputs()` should happen once per frame before reading `GetInputsSnapshot()`.
* **Snapshot is the contract**: avoid calling `glfwGetKey` and friends in gameplay code; keep all input access through the snapshot.
* **Mouse capture**: use capture for gameplay camera, disable it for UI screens (inventory, menus, console, etc.).
* **Unregister inputs**: if you support remapping or context-specific bindings, call `UnregisterInput(id)` and register new ones.

---

## Quick reference

### Typical per-frame flow

```cpp
glfwPollEvents();
inputs.PoolInputs();
auto snap = inputs.GetInputsSnapshot();

// read snap->Mouse / snap->Framebuffer / snap->GetKeyState(id)
```

### Accessing key state

```cpp
KeyState ks = snap->GetKeyState(myInputId);
if (ks.IsPressed) { ... }
if (ks.IsDoublePressed) { ... }
```

### Accessing mouse delta

```cpp
if (snap->Mouse.MovementOffsetChanged)
{
    double dx = snap->Mouse.Xoffset;
    double dy = snap->Mouse.Yoffset;
}
```
