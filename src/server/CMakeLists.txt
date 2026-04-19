# Minimum CMake version requirement
cmake_minimum_required(VERSION 3.20)

# Project declaration and language setup
project(onion_voxel_server LANGUAGES CXX)

# Path to server application icon
set(ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../assets/app_icons/Vox_Server_Icon.ico")

# Generate Windows resource file from template
configure_file(
    app_icon.rc.in
    ${CMAKE_CURRENT_BINARY_DIR}/app_icon.rc
    @ONLY
)

# Define server library and its source files
add_library(onion_voxel_server
    "src/Server.cpp"

	"src/network_server/NetworkServer.cpp"
)

# Link dependencies to the server library
target_link_libraries(onion_voxel_server
    PUBLIC
        enet::enet
		nlohmann_json::nlohmann_json

		onion::event
		onion::threadsafequeue
		onion::logger
		onion_voxel_shared
)

# Link additional Windows system libraries
if (WIN32)
    target_link_libraries(onion_voxel_server
        PUBLIC
            winmm
            ws2_32
    )
endif()

# Ensure correct __cplusplus macro behavior on MSVC
target_compile_options(onion_voxel_server PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>
)

# Add include directories for the library
target_include_directories(onion_voxel_server
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Require C++20 standard for the library
target_compile_features(onion_voxel_server PUBLIC cxx_std_20)

# Enable compiler warnings
target_compile_options(onion_voxel_server
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)

# Enforce strict C++ standard settings
set_target_properties(onion_voxel_server PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Define server executable
add_executable(OnionVoxelServer
	"src/main.cpp"
)

# Link executable with server library
target_link_libraries(OnionVoxelServer
    PRIVATE
        onion_voxel_server
)

# Add Windows-specific resource file (icon)
if (WIN32)
    target_sources(OnionVoxelServer PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/app_icon.rc)
endif()

# Install executable to server folder
install(TARGETS OnionVoxelServer
    RUNTIME DESTINATION server
)

# Install server configuration file
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/config_server.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/readme.txt"
    DESTINATION server
)

# Install server thumbnail asset
install(FILES
    "${CMAKE_SOURCE_DIR}/assets/textures/ServerThumbnail.png"
    DESTINATION server
)
