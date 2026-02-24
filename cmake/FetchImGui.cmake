# FetchImGui.cmake
# ---------------------------------------------------------------------------
# Fetch and expose Dear ImGui as target imgui::imgui
# Optional: build selected backends (e.g. GLFW + OpenGL3)
# ---------------------------------------------------------------------------

if (TARGET imgui::imgui)
    return()
endif()

include(FetchContent)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  # Pin a known commit or tag for reproducibility.
  # ImGui doesn't use semantic version tags consistently, so commit pin is common.
  GIT_TAG v1.90.9-docking
)

FetchContent_MakeAvailable(imgui)

# imgui_SOURCE_DIR is provided by FetchContent
set(IMGUI_DIR ${imgui_SOURCE_DIR})

add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp

    # Optional extras
    ${IMGUI_DIR}/imgui_demo.cpp
)

add_library(imgui::imgui ALIAS imgui)

target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
)

# -------------------------
# Optional backends section
# -------------------------
# Example: GLFW + OpenGL3 backend
# You must link glfw + OpenGL yourself (or via your own targets).

option(IMGUI_WITH_GLFW_OPENGL3 "Build ImGui GLFW+OpenGL3 backend" ON)

if (IMGUI_WITH_GLFW_OPENGL3)
    target_sources(imgui PRIVATE
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    )

    target_include_directories(imgui PUBLIC
        ${IMGUI_DIR}/backends
    )

    # You typically need these definitions for the OpenGL3 backend:
    # Pick a GLSL version matching your renderer.
    target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)

	target_link_libraries(imgui
        PUBLIC
            glfw
    )
endif()
