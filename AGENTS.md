# AGENTS.md — OnionVoxel

## Agent behaviour — clarification protocol

Before starting any non-trivial task, ask clarifying questions until confidence in the user's intent reaches ~95%. Do not begin implementation while ambiguity remains.

- Ask about **implementation details** whenever multiple reasonable approaches exist (e.g. where to place new code, how a system should interact with existing ones, expected behaviour at edge cases).
- Ask about **scope** if the request could mean a small change or a large refactor.
- Ask about **target** (client, server, shared, or all three) when the request does not make it obvious.
- Batch all open questions into a **single message** — do not ask one question, wait, then ask another.
- If a question has an obvious default that the user likely intends, state your assumption and ask only if you are not confident.
- Once intent is clear, state the plan concisely and proceed.

---

## What this is

C++20 voxel game (Minecraft-like). CMake + Ninja build. No Node, no Python, no test framework. Windows-primary; Linux supported but untested in CI.

Three CMake targets:
- `OnionVoxel` — client executable (`src/client/`)
- `OnionVoxelServer` — server executable (`src/server/`)
- `onion_voxel_shared` — static library shared by both (`src/shared/`)

Six internal libraries live in `deps/` as git submodules (`onion::event`, `onion::logger`, `onion::timer`, `onion::threadpool`, `onion::threadsafequeue`, `onion::datetime`), all from `github.com/nicolas-vtn/`.

---

## Build commands

```sh
# First clone — always recurse submodules
git clone --recurse-submodules <url>
# or after a plain clone
git submodule update --init --recursive
```

Visual Studio users: open the folder; `CMakeSettings.json` defines three preset configs (`x64-Debug`, `x64-Release`, `x64-Release-Deploy`) — all use Ninja + MSVC x64. VS owns the CMake cache under `out/build/<config-name>/`.

`DEPLOY_BUILD=ON` enables `/O2`, strips debug info, and applies `/OPT:REF /OPT:ICF`. Do not set it for development builds.

### Building from the command line (agent use)

Do **not** run `cmake -S . -B ...` against a directory that Visual Studio manages — it will overwrite the cache with wrong settings.

To build without disturbing the VS cache, activate the MSVC x64 environment first, then drive the existing cache:

```powershell
cmd /c "`"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat`" amd64 && cmake --build out/build/x64-Debug 2>&1"
```

- `vcvarsall.bat amd64` matches the `msvc_x64_x64` environment VS uses — without it, the linker resolves x86 CRT/SDK libs and the build fails with hundreds of LNK2019 errors.
- `cmake --build` drives Ninja against the existing cache; it never modifies `CMakeCache.txt`.
- `2>&1` merges stderr so warnings and errors appear in one stream.

---

## No tests, no linter

There is no test suite (no CTest, no GTest, no catch2). There is no `.clang-tidy` config. CI only runs on tag push and only builds a release zip — it does not gate PRs.

Verification workflow: **build → run manually**.

---

## Code style (enforced by `.clang-format`)

Run `clang-format` before submitting changes to C++ files.

- BasedOnStyle: LLVM, Language: C++20
- **Allman braces** — every `{` on its own line
- **Tabs, width 4** for indentation
- Line length: 120
- Pointer/reference alignment: Left (`T* p`, `T& r`)
- Namespace indentation: All
- No short `if`/loops on single line

```sh
# Format a file in-place
clang-format -i src/path/to/file.cpp
```

`.editorconfig` requires `lf` line endings, `utf-8`, and a trailing newline.

---

## Key architecture facts

- **Namespace**: everything under `namespace onion::voxel`; internal deps use `namespace onion`.
- **Chunk size**: `CHUNK_SIZE = 64` (XZ); chunks are column-based, variable height. Sub-chunks are 64×64×64 with per-chunk palette compression. Sub-chunks are lazily allocated — an all-air or mono-block sub-chunk holds a single index with no heap allocation.
- **Threading**: render loop on a `std::jthread` owned by `Renderer` — all OpenGL calls must happen on that thread. Network handlers run on a separate dispatch thread. World gen and mesh building use thread pools. Per-chunk `std::shared_mutex` guards cross-thread chunk access. → See [`docs/threading.md`](docs/threading.md).
- **Networking**: ENet over UDP. Messages are a `std::variant<...>` framed with a `MessageHeader` (type enum + ClientHandle). Serialized with Cereal. Adding a message type requires touching 5 files. → See [`docs/networking.md`](docs/networking.md).
- **Assets at runtime**: `AssetsManager` resolves paths relative to the executable directory (not CWD). Static methods (`AssetsManager::GetShadersDirectory()` etc.) are usable before any `AssetsManager` instance is constructed. After `cmake --install`, client ships with the full `assets/` tree; server ships with only `assets/blockstates.zip` and `assets/models.zip`.
- **Runtime config files**: `config_client.json` (PlayerName, UUID) and `config_server.json` (Port 7777, Seed, MOTD, SimulationDistance) are created/read from the CWD at launch.

---

## Codegen — version injection

`cmake/GetGitVersion.cmake` runs `git describe --tags --always --dirty` at **configure time** and generates `version.hpp`/`version.cpp` into `${CMAKE_BINARY_DIR}/generated/`. If you add a tag, reconfigure to pick it up.

---

## External dependencies

All pulled via CMake FetchContent at configure time (no lockfile):
GLFW 3.4, GLM, STB Image, Nlohmann JSON, Dear ImGui, ENet v1.3.18, Cereal v1.3.2, FastNoiseLite, Miniz.

`external/glad/` is vendored directly (not fetched). Do not delete it.

On Linux, `CURL::libcurl` must be installed system-wide — it is linked into `onion_voxel_shared` for skin downloading (Windows uses WinHTTP instead).

---

## CI (`.github/workflows/release.yml`)

Runs only on `v*` tag push. Windows-only runner. Steps: configure with `DEPLOY_BUILD=ON` → build → install → zip → GitHub Release. There is no PR CI.

---

## What does not exist (don't go looking)

- No test framework or test directory
- No pre-commit hooks or task runner (no Makefile, no Taskfile)
- No `.clang-tidy` config
- No `opencode.json` or other AI instruction files
- No audio system (planned, not implemented)
- No Linux CI
