cmake_minimum_required(VERSION 3.20)

project(onion_voxel
    VERSION 0.1.0
    DESCRIPTION "A C++ Minecraft clone, (almost) from scratch."
    LANGUAGES C CXX
)

set(ONION_BUILD_DEMO OFF CACHE BOOL "Compile Dependencies Demos" FORCE)

# Modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Dependencies (GLFW, GLAD, GLM, stb, ...)
include(FetchContent)

include(FetchGLFW)
include(FetchGLM)
include(FetchSTB)
include(FetchNlohmannJson)
include(FetchImGui)
include(FetchENet)
include(FetchCereal)
include(FetchFastNoiseLite)
include(FetchMiniz)

include(GetGitVersion)

get_git_version(GIT_VERSION)
message(STATUS "Git Version: ${GIT_VERSION}")

# Ensure CMake reconfigures when .git/index changes (e.g., new commits, branch changes)
set_property(DIRECTORY APPEND PROPERTY
    CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/.git/index"
)

add_subdirectory(external/glad)

# Internal Dependencies
add_subdirectory(deps/onion-datetime)
add_subdirectory(deps/onion-event)
add_subdirectory(deps/onion-logger)
add_subdirectory(deps/onion-timer)
add_subdirectory(deps/onion-threadsafequeue)
add_subdirectory(deps/onion-threadpool)

add_subdirectory(src/shared)
add_subdirectory(src/client)
add_subdirectory(src/server)
