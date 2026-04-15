#include "Utils.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <random>
#include <string>
#include <cstdlib>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <Windows.h>
#else
    #include <unistd.h>
    #include <limits.h>
#endif

namespace onion::voxel::Utils
{

std::filesystem::path GetExecutableDirectory()
{
#ifdef _WIN32
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::filesystem::path executablePath(buffer);
    return executablePath.parent_path();
#else
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);

    if (count == -1)
        return std::filesystem::current_path();

    std::filesystem::path executablePath(std::string(result, count));
    return executablePath.parent_path();
#endif
}

void OpenDirectoryInFileExplorer(const std::filesystem::path& directory)
{
#ifdef _WIN32
    std::string command = "explorer \"" + directory.string() + "\"";
#elif defined(__linux__)
    // xdg-open is the standard cross-desktop way
    std::string command = "xdg-open \"" + directory.string() + "\"";
#else
    return;
#endif

    std::system(command.c_str());
}

std::string SanitizeFileName(const std::string& name)
{
    std::string result;
    result.reserve(name.size());

    for (unsigned char ch : name)
    {
        if (ch < 32 || ch == '<' || ch == '>' || ch == ':' || ch == '"' ||
            ch == '/' || ch == '\\' || ch == '|' || ch == '?' || ch == '*' || ch == ' ')
        {
            result += '_';
        }
        else
        {
            result += static_cast<char>(ch);
        }
    }

    while (!result.empty() && (result.back() == ' ' || result.back() == '.'))
    {
        result.pop_back();
    }

    if (result.empty())
    {
        return "untitled";
    }

    auto toUpper = [](std::string s)
    {
        std::transform(
            s.begin(), s.end(), s.begin(),
            [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        return s;
    };

    std::string baseName = result;
    size_t dotPos = baseName.find('.');
    if (dotPos != std::string::npos)
    {
        baseName = baseName.substr(0, dotPos);
    }

    std::string upperBase = toUpper(baseName);

#ifdef _WIN32
    // Only relevant on Windows
    if (upperBase == "CON" || upperBase == "PRN" || upperBase == "AUX" || upperBase == "NUL" ||
        upperBase == "COM1" || upperBase == "COM2" || upperBase == "COM3" || upperBase == "COM4" ||
        upperBase == "COM5" || upperBase == "COM6" || upperBase == "COM7" || upperBase == "COM8" ||
        upperBase == "COM9" || upperBase == "LPT1" || upperBase == "LPT2" || upperBase == "LPT3" ||
        upperBase == "LPT4" || upperBase == "LPT5" || upperBase == "LPT6" || upperBase == "LPT7" ||
        upperBase == "LPT8" || upperBase == "LPT9")
    {
        result = "_" + result;
    }
#endif

    return result;
}

std::string GenerateUUID()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    std::string uuid;
    for (int i = 0; i < 32; i++)
        uuid += "0123456789abcdef"[dis(gen)];

    return uuid;
}

} // namespace onion::voxel::Utils