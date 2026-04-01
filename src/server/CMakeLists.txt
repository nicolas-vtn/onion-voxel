cmake_minimum_required(VERSION 3.20)

project(onion_voxel_server LANGUAGES CXX)

# ---- Library ----
add_library(onion_voxel_server
    "src/Server.cpp"

	"src/network_server/NetworkServer.cpp"
)

target_link_libraries(onion_voxel_server
    PUBLIC
        enet::enet
		winmm
		ws2_32
		nlohmann_json::nlohmann_json

		onion::event
		onion::threadsafequeue
		onion::logger
		onion_voxel_shared
)

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
