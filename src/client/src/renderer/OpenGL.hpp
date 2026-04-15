#pragma once

#ifdef _WIN32
    // Include Windows.h first
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <Windows.h>
#endif

// Include glad (after Windows.h)
#include <glad/glad.h>

// Include GLFW (after glad)
#include <GLFW/glfw3.h>
