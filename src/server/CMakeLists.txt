cmake_minimum_required(VERSION 3.20)

project(onion_voxel_server LANGUAGES CXX)

# Copy assets to build directory
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/../../assets/textures/ServerThumbnail.png" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/config_server.json" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

set(ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../assets/app_icons/Vox_Server_Icon.ico")

configure_file(
    app_icon.rc.in
    ${CMAKE_CURRENT_BINARY_DIR}/app_icon.rc
    @ONLY
)

# ---- Library ----
add_library(onion_voxel_server
    "src/Server.cpp"

	"src/network_server/NetworkServer.cpp"
)

target_link_libraries(onion_voxel_server
    PUBLIC
        enet::enet
		nlohmann_json::nlohmann_json

		onion::event
		onion::threadsafequeue
		onion::logger
		onion_voxel_shared
)

if (WIN32)
    target_link_libraries(onion_voxel_server
        PUBLIC
            winmm
            ws2_32
    )
endif()

target_compile_options(onion_voxel_server PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>
)

target_include_directories(onion_voxel_server
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(onion_voxel_server PUBLIC cxx_std_20)

target_compile_options(onion_voxel_server
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)

set_target_properties(onion_voxel_server PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

add_executable(OnionVoxelServer
	"src/main.cpp"
)

target_link_libraries(OnionVoxelServer
    PRIVATE
        onion_voxel_server
)

if (WIN32)
    target_sources(OnionVoxelServer PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/app_icon.rc)
endif()
