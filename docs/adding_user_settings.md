# Adding a UserSetting

This document walks through every file that must be touched to add a new persistent
user setting, using `WailaEnabled` as the concrete example throughout.

---

## Two categories — decide first

Before writing any code, determine which category the new setting falls into:

| Category | Description | `ApplyUserSettings` branch needed? | Local mirror in `Renderer`? |
|---|---|---|---|
| **Live-read** | Consumed by reading `EngineContext::Get().Settings()` inline each frame. No side-effect beyond storing the value. | No | No |
| **Apply-once** | Drives an imperative side-effect: an OpenGL/GLFW call, a subsystem method, or an event. Must be applied immediately when changed and once at startup. | Yes | Only if read in the hot render loop |

Examples:

| Setting | Category | Side-effect |
|---|---|---|
| `WailaEnabled` | Live-read | None — `HudPanel::Render` reads it inline |
| `VSyncEnabled` | Apply-once | `glfwSwapInterval(0 or 1)` |
| `MaxFPS` | Apply-once | Cached into `Renderer::m_MaxFps` for the frame-cap loop |
| `RenderDistance` | Apply-once | `WorldManager::SetChunkPersistanceDistance(...)` |
| `FOV` | Apply-once | `Camera::SetFov(...)` |

---

## 13-step checklist

| # | File | What to do |
|---|---|---|
| 1 | `VideoSettings.hpp` | Add field with default value; add key to `to_json`; add `contains` guard in `from_json` |
| 2 | `GuiElement.hpp` — `UserSettingsChangedEventArgs` | Add `bool SettingName_Changed = false`; set it inside the `if (allChanged)` branch |
| 3 | `VideoSettingsPanel.hpp` | Declare control member(s) (`Button`, `Slider`, `Label`) and handler signature |
| 4 | `VideoSettingsPanel.cpp` — constructor | Forward-construct controls in initializer list; set static label text in body |
| 5 | `VideoSettingsPanel.cpp` — `Render()` | Read settings from `EngineContext`, compute layout, set dynamic text, call `Render()` on each control; ensure the last new control updates `contentHeight` |
| 6 | `VideoSettingsPanel.cpp` — `Initialize()` | Call `control.Initialize()`; seed initial slider value if applicable |
| 7 | `VideoSettingsPanel.cpp` — `Delete()` | Call `control.Delete()` |
| 8 | `VideoSettingsPanel.cpp` — `ReloadTextures()` | Call `control.ReloadTextures()` |
| 9 | `VideoSettingsPanel.cpp` — `SubscribeToControlEvents()` | Subscribe to `EvtClick` / `EvtValueChanged`; store the returned `EventHandle` in `m_EventHandles` |
| 10 | `VideoSettingsPanel.cpp` — `Handle_*` | Read snapshot, mutate field, build `UserSettingsChangedEventArgs`, set dirty flag, trigger `EvtUserSettingsChanged` |
| 11 | `Renderer.cpp` — `ApplyUserSettings()` | **Live-read:** nothing needed — `UpdateSettings` already runs unconditionally. **Apply-once:** add `if (args.SettingName_Changed) { ... }` |
| 12 | `Renderer.hpp` | **Hot-loop only:** add a local mirror member if the render loop reads the value every frame |
| 13 | Consumer | Read `EngineContext::Get().Settings().Video.SettingName` inline at the point of use |

---

## Step-by-step walkthrough

### Step 1 — `VideoSettings.hpp`

`src/client/src/user_settings/video_settings/VideoSettings.hpp`

Add the field to the struct with its default value:

```cpp
struct VideoSettings
{
    // existing fields ...
    bool WailaEnabled = true;   // <-- new
};
```

Add the key to `to_json`:

```cpp
j = BasicJsonType{ ..., {"WailaEnabled", s.WailaEnabled} };
```

Add a guarded entry in `from_json`. The `contains` check is mandatory — it makes
the field silently default when loading an older config file that predates the key:

```cpp
if (j.contains("WailaEnabled"))
    j.at("WailaEnabled").get_to(s.WailaEnabled);
```

The on-disk file (`user_settings.json`, created at runtime in the CWD) will gain the
new key on the next save. Old files without the key keep the struct default.

---

### Step 2 — `UserSettingsChangedEventArgs` (`GuiElement.hpp`)

`src/client/src/renderer/gui/GuiElement.hpp`

This class is the event payload that travels from the UI panel up to `Renderer`.
Each setting gets one dirty flag so `ApplyUserSettings` knows what actually changed.

Add the flag as a member:

```cpp
bool WailaEnabled_Changed = false;
```

Add it to the `allChanged = true` constructor branch. This branch is used at startup
(see Step 11) to trigger a full apply-pass. If you forget this, the setting will not
be applied correctly when the game first launches:

```cpp
if (allChanged)
{
    // existing flags ...
    WailaEnabled_Changed = true;   // <-- new
}
```

---

### Steps 3–10 — `VideoSettingsPanel`

`src/client/src/renderer/gui/panels/video_settings_panel/VideoSettingsPanel.hpp`
`src/client/src/renderer/gui/panels/video_settings_panel/VideoSettingsPanel.cpp`

#### Header — declare members and handler (Step 3)

```cpp
// in the Controls section:
Label  m_WailaTitle_Label;
Button m_Waila_Button;

// in the private handlers section:
void Handle_Waila_Click(const Button& sender);
```

#### Constructor — forward-construct and configure (Step 4)

In the initializer list:

```cpp
m_WailaTitle_Label(name + "_WailaTitle_Label"),
m_Waila_Button(name + "_Waila_Button"),
```

In the body (static text only — dynamic text goes in `Render()`):

```cpp
m_WailaTitle_Label.SetText("WAILA (What Am I Looking At)");
m_WailaTitle_Label.SetTextAlignment(Font::eTextAlignment::Left);
```

#### `Render()` — layout and dynamic text (Step 5)

The panel reads the current settings snapshot fresh every frame:

```cpp
UserSettings userSettings = EngineContext::Get().Settings();
```

Follow the existing layout pattern: compute a section title position
(`spacingYBetweenSections` below the last control of the previous section),
create a `TableLayout`, place the button in the first cell.

For a toggle button the dynamic text pattern is:

```cpp
const std::string wailaButtonText =
    "WAILA : " + std::string(userSettings.Video.WailaEnabled ? "On" : "Off");
m_Waila_Button.SetText(wailaButtonText);
```

**Do not forget** to update `contentHeight` at the end of `Render()` to use the
bottom edge of the last new control, otherwise the scroller will not scroll far
enough to reveal it:

```cpp
const int contentHeight =
    wailaButtonPosition.y + (wailaButtonSize.y / 2) - topLeftCorner.y;
m_Scroller.SetScrollAreaHeight(contentHeight);
```

#### `Initialize()` / `Delete()` / `ReloadTextures()` (Steps 6–8)

Every control added to the panel must appear in all three lifecycle methods.
Missing any one causes either a crash on resource-pack reload or a GPU resource leak:

```cpp
// Initialize
m_WailaTitle_Label.Initialize();
m_Waila_Button.Initialize();

// Delete
m_WailaTitle_Label.Delete();
m_Waila_Button.Delete();

// ReloadTextures
m_WailaTitle_Label.ReloadTextures();
m_Waila_Button.ReloadTextures();
```

For a `Slider`, `Initialize()` is also where you seed the initial value from settings:

```cpp
m_MySlider.SetValue(settings.Video.MySliderValue - s_MySlider_MinValue);
```

#### `SubscribeToControlEvents()` (Step 9)

```cpp
m_EventHandles.push_back(
    m_Waila_Button.EvtClick.Subscribe(
        [this](const Button& sender) { Handle_Waila_Click(sender); }));
```

The `EventHandle` **must** be stored in `m_EventHandles`. The destructor clears this
vector, which releases all subscriptions. If you store the handle elsewhere (or
discard it), the lambda will fire on a destroyed `this`.

#### Handler (Step 10)

The canonical pattern:

```cpp
void VideoSettingsPanel::Handle_Waila_Click(const Button& sender)
{
    (void) sender;

    UserSettings settings = EngineContext::Get().Settings();
    settings.Video.WailaEnabled = !settings.Video.WailaEnabled;

    UserSettingsChangedEventArgs args(settings);
    args.WailaEnabled_Changed = true;   // set ONLY the relevant flag

    EvtUserSettingsChanged.Trigger(args);
}
```

1. Read the full current snapshot from `EngineContext` — never mutate a cached copy.
2. Mutate only the relevant field.
3. Construct `UserSettingsChangedEventArgs` with `allChanged = false` (the default).
4. Set only the one dirty flag for this setting.
5. Fire `EvtUserSettingsChanged`. This propagates via `Gui` to `Renderer::ApplyUserSettings`.

---

### Step 11 — `Renderer::ApplyUserSettings()` (`Renderer.cpp`)

`src/client/src/renderer/Renderer.cpp`

`ApplyUserSettings` always begins with:

```cpp
EngineContext::Get().UpdateSettings(args.NewSettings);
```

This line alone is sufficient for **live-read** settings. `WailaEnabled` needs nothing
else here — every consumer that reads `EngineContext::Get().Settings()` inline will
see the new value on the very next frame.

For **apply-once** settings, add a conditional branch after the `UpdateSettings` call:

```cpp
if (args.VSyncEnabled_Changed)
{
    m_IsVsyncEnabled = settings.Video.VSyncEnabled;
    glfwSwapInterval(m_IsVsyncEnabled ? 1 : 0);
}
```

#### Startup — `allChanged = true`

At startup, `Renderer::Start()` calls:

```cpp
UserSettingsChangedEventArgs args(settings, /*allChanged=*/true);
args.ResourcePack_Changed = false;  // handled separately (OpenGL not ready yet)
ApplyUserSettings(args);
```

The `allChanged = true` constructor sets every `*_Changed` flag to `true`. This
triggers every apply-once branch once at boot. Live-read settings pass through
harmlessly — their `UpdateSettings` call already ran, and there is no branch to
execute. This is why Step 2 (adding the flag to the `allChanged` branch) matters
even for live-read settings: the flag must exist there to remain consistent, even
though no branch in `ApplyUserSettings` reads it.

---

### Step 12 — Local mirror in `Renderer.hpp` (apply-once + hot loop only)

`src/client/src/renderer/Renderer.hpp`

Only needed when the render-loop hot path tests the value every frame and calling
`EngineContext::Get().Settings()` would add unwanted overhead:

```cpp
bool     m_IsVsyncEnabled = true;   // mirrors VSyncEnabled
uint32_t m_MaxFps = 60;             // mirrors MaxFPS
```

`WailaEnabled` does **not** have a mirror — `HudPanel::Render` reads it once per
frame via `EngineContext::Get().Settings()`, which is a mutex-guarded copy operation.
For a single `bool` read per frame that cost is negligible.

---

### Step 13 — Consumer

Any system that needs to react to the setting simply reads it inline:

```cpp
if (EngineContext::Get().Settings().Video.WailaEnabled)
{
    // render WAILA tooltip
}
```

No event subscription, no cached member, no mirror. The value is always current
because `ApplyUserSettings` called `UpdateSettings` before this frame ran.

---

## What not to forget

- **`from_json` must use `contains()` guard** — without it, loading a config file
  that predates the key throws a JSON exception and falls back to all defaults,
  silently discarding all other user preferences.

- **`allChanged` branch in `UserSettingsChangedEventArgs`** — if you omit the flag
  from the `if (allChanged)` block, the setting will not be applied at startup for
  apply-once settings, and the `allChanged = true` constructor will be silently
  inconsistent for live-read ones.

- **All four lifecycle methods** — `Initialize`, `Delete`, `ReloadTextures`, and
  `Render`. A missing `Delete` leaks GPU resources. A missing `ReloadTextures`
  causes the control to go blank or crash after a resource-pack switch.

- **`EventHandle` in `m_EventHandles`** — discarding the handle returned by
  `Subscribe` causes an immediate dangling-reference crash the first time the event
  fires after the subscription is destroyed.

- **`contentHeight` in `Render()`** — if the new control is the last one in the
  scrollable area and you forget to update `contentHeight`, the scroller clips it.

- **Apply-once settings need the branch in `ApplyUserSettings` AND the `allChanged`
  entry** — without the branch, the setting is never applied. Without the `allChanged`
  entry, it is not applied at startup.
