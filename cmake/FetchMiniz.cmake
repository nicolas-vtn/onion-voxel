# FetchMiniz.cmake
# ---------------------------------------------------------------------------
# Fetch and expose miniz as target miniz::miniz
# ---------------------------------------------------------------------------

if (TARGET miniz::miniz)
    return()
endif()

include(FetchContent)

FetchContent_Declare(
    miniz
    GIT_REPOSITORY https://github.com/richgel999/miniz.git
    GIT_TAG 3.1.1 # latest stable tag at the time of writing
)

FetchContent_MakeAvailable(miniz)

target_include_directories(miniz
    PUBLIC
        ${miniz_SOURCE_DIR}
)

# Create namespace alias
add_library(miniz::miniz ALIAS miniz)
