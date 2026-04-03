# GetGitVersion.cmake
#
# Usage:
#   include(GetGitVersion)
#   get_git_version(GIT_VERSION)
#
# Result:
#   GIT_VERSION = "v1.2.3_dirty" (example)

function(get_git_version OUT_VAR)
    find_package(Git QUIET)

    if(NOT GIT_FOUND)
        set(${OUT_VAR} "unknown" PARENT_SCOPE)
        return()
    endif()

    # --- Get version (tag or commit hash) ---
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_DESCRIBE
        RESULT_VARIABLE GIT_RESULT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(NOT GIT_RESULT EQUAL 0)
        set(${OUT_VAR} "unknown" PARENT_SCOPE)
        return()
    endif()

    # --- Normalize dirty suffix (optional tweak) ---
    # git already appends "-dirty", replace with "_dirty" if you prefer
    string(REPLACE "-dirty" "_dirty" GIT_VERSION "${GIT_DESCRIBE}")

    set(${OUT_VAR} "${GIT_VERSION}" PARENT_SCOPE)
endfunction()
