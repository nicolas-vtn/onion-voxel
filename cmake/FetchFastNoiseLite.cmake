include(FetchContent)

# Prevent multiple declarations
if(TARGET FastNoiseLite)
    return()
endif()

FetchContent_Declare(
    fastnoiselite
    GIT_REPOSITORY https://github.com/Auburn/FastNoiseLite.git
    GIT_TAG master
)

FetchContent_GetProperties(fastnoiselite)

if(NOT fastnoiselite_POPULATED)
    FetchContent_Populate(fastnoiselite)

    add_library(FastNoiseLite INTERFACE)

    target_include_directories(FastNoiseLite
        INTERFACE
            ${fastnoiselite_SOURCE_DIR}/Cpp
    )
endif()
