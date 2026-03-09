cmake_minimum_required(VERSION 3.20)

project(onion_voxel_shared LANGUAGES CXX)

# ---- Library ----
add_library(onion_voxel_shared
    "dummy.cpp"
 
 "shared/world/chunk/Chunk.cpp"
 "shared/world/world_manager/WorldManager.cpp"

 "shared/world/chunk/SubChunk.cpp" "shared/world/block/Block.cpp" "shared/network_messages/Serializer.cpp")

target_link_libraries(onion_voxel_shared
	PUBLIC
		cereal::cereal
		glm

		onion::event
)

target_include_directories(onion_voxel_shared
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(onion_voxel_shared PUBLIC cxx_std_20)

set_target_properties(onion_voxel_shared PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
