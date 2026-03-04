# FetchCereal.cmake
# ---------------------------------------------------------------------------
# Fetch and expose Cereal as target cereal::cereal
# ---------------------------------------------------------------------------

if (TARGET cereal::cereal)
    return()
endif()

include(FetchContent)

FetchContent_Declare(
  cereal
  GIT_REPOSITORY https://github.com/USCiLab/cereal.git
  GIT_TAG v1.3.2   # Latest stable release (recommended)
)

set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SKIP_PERFORMANCE_COMPARISON ON CACHE BOOL "" FORCE)
set(CEREAL_INSTALL OFF CACHE BOOL "" FORCE)
set(JUST_INSTALL_CEREAL ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(cereal)

# Older versions expose target 'cereal'
if (TARGET cereal AND NOT TARGET cereal::cereal)
    add_library(cereal::cereal ALIAS cereal)
endif()
