# cmake/FetchGLFW.cmake
# ---------------------------------------------------------------------------
# Fetch and make GLFW available as a CMake target `glfw`
# ---------------------------------------------------------------------------

# Prevent double inclusion
if (TARGET glfw)
    return()
endif()

include(FetchContent)

# Optional: disable extras to speed up configuration
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)

# Fetch GLFW from GitHub
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
)

FetchContent_MakeAvailable(glfw)