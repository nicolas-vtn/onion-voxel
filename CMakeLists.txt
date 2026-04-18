cmake_minimum_required(VERSION 3.20)

project(onion_voxel
    VERSION 0.1.0
    DESCRIPTION "A C++ Minecraft clone, (almost) from scratch."
    LANGUAGES C CXX
)

# Deploy Build
if(DEPLOY_BUILD)
    # Compiler
    add_compile_options(
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Release>:/DNDEBUG>
    )

    # Remove debug info
    string(REPLACE "/Zi" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "/ZI" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

    # Linker
    add_link_options(
        $<$<CONFIG:Release>:/INCREMENTAL:NO>  # removes .ilk
        $<$<CONFIG:Release>:/DEBUG:NONE>      # no pdb
        $<$<CONFIG:Release>:/OPT:REF>
        $<$<CONFIG:Release>:/OPT:ICF>
    )
endif()

# Standalone executable (static MSVC runtime)
if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

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
