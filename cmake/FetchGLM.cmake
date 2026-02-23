# FetchGLM.cmake
# ---------------------------------------------------------------------------
# Fetch and expose GLM (OpenGL Mathematics) as target glm::glm
# ---------------------------------------------------------------------------

if (TARGET glm::glm)
    return()
endif()

include(FetchContent)

# Fetch GLM from its official repository
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.1  # or any newer stable tag
)

# Automatically fetch and add it
FetchContent_MakeAvailable(glm)

# GLM versions prior to 1.0.1 define target 'glm', so add an alias for consistency
if (TARGET glm AND NOT TARGET glm::glm)
    add_library(glm::glm ALIAS glm)
endif()
