# FetchNlohmannJson.cmake
# ---------------------------------------------------------------------------
# Fetch and expose nlohmann/json as target nlohmann_json::nlohmann_json
# ---------------------------------------------------------------------------

if (TARGET nlohmann_json::nlohmann_json)
    return()
endif()

include(FetchContent)

# Fetch nlohmann/json from its official repository
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3  # Use latest stable release if needed
)

# Automatically fetch and add it
FetchContent_MakeAvailable(nlohmann_json)

# Older versions define target 'nlohmann_json' only
if (TARGET nlohmann_json AND NOT TARGET nlohmann_json::nlohmann_json)
    add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
endif()
