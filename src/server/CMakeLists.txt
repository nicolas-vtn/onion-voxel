cmake_minimum_required(VERSION 3.20)

project(onion_voxel_server LANGUAGES CXX)

# ---- Library ----
add_library(onion_voxel_server
    "src/Server.cpp"

	"src/network_server/NetworkServer.cpp"
	"src/world_generator/WorldGenerator.cpp"
)

target_link_libraries(onion_voxel_server
    PUBLIC
        enet::enet
		winmm
		ws2_32
		nlohmann_json::nlohmann_json
		FastNoiseLite

		onion::event
		onion::threadsafequeue
		onion::threadpool
		onion_voxel_shared		
)

target_include_directories(onion_voxel_server
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(onion_voxel_server PUBLIC cxx_std_20)

set_target_properties(onion_voxel_server PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
