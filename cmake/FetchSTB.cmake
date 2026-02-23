# FetchSTB.cmake
# ---------------------------------------------------------------------------
# Fetch and expose stb single-header libraries as target stb::stb
# ---------------------------------------------------------------------------

if (TARGET stb::stb)
    return()
endif()

include(FetchContent)

# Allow overriding the exact commit/tag from the command line/caches
set(FETCH_STB_TAG "master" CACHE STRING "stb git tag/branch/commit to fetch")

FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG        ${FETCH_STB_TAG}
)

# stb has no CMakeLists, so we populate and make our own interface target
FetchContent_GetProperties(stb)
if (NOT stb_POPULATED)
  FetchContent_Populate(stb)
endif()

# Create a header-only interface target that exports the include path
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE
  $<BUILD_INTERFACE:${stb_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>  # (optional, if you later install)
)

# Canonical alias
add_library(stb::stb ALIAS stb)
