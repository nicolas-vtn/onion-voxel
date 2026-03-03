# FetchENet.cmake
# ---------------------------------------------------------------------------
# Fetch and expose ENet as target enet::enet
# ---------------------------------------------------------------------------

if (TARGET enet::enet)
    return()
endif()

include(FetchContent)

FetchContent_Declare(
    enet
    GIT_REPOSITORY https://github.com/lsalzman/enet.git
    GIT_TAG v1.3.18
)

set(ENET_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ENET_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ENET_INSTALL OFF CACHE BOOL "" FORCE)
set(ENET_SHARED OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(enet)

# ENet creates target "enet"
# Force include directories to be PUBLIC (important)
target_include_directories(enet
    PUBLIC
        $<BUILD_INTERFACE:${enet_SOURCE_DIR}/include>
)

# Create namespace alias
add_library(enet::enet ALIAS enet)
