cmake_minimum_required(VERSION 3.20)

project(onion_voxel_shared LANGUAGES CXX)

# ---- Library ----
add_library(onion_voxel_shared
 
 "shared/world/chunk/Chunk.cpp"
 "shared/world/chunk/SubChunk.cpp"
 "shared/world/block/Block.cpp"
 "shared/world/block/BlockState.cpp" 
 "shared/world/block/BlockModel.cpp"
 "shared/world/block/BlockstateRegistry.cpp"
 "shared/world/block/BlockPlacementResolver.cpp"

 "shared/world/world_manager/WorldManager.cpp"
 "shared/world/world_generator/WorldGenerator.cpp"
 "shared/world/world_save/WorldSave.cpp"
 "shared/world/raycast/Raycast.cpp"

 "shared/data_transfer_objects/serializer/SerializerDTO.cpp"

 "shared/zip_archive/ZipArchive.cpp"

 "shared/entities/entity_manager/EntityManager.cpp"
 "shared/entities/entity/Entity.cpp"
 "shared/entities/entity/player/Player.cpp"

 "shared/physics/PhysicsEngine.cpp"

 "shared/utils/Utils.cpp"
 )

target_link_libraries(onion_voxel_shared
	PUBLIC
		cereal::cereal
		glm
		miniz
		FastNoiseLite
		nlohmann_json::nlohmann_json

		onion::threadsafequeue
		onion::threadpool
		onion::event
		onion::timer
		onion::datetime
)

target_compile_options(onion_voxel_shared PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>
)

target_include_directories(onion_voxel_shared
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(onion_voxel_shared PUBLIC cxx_std_20)

target_compile_options(onion_voxel_shared
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)

set_target_properties(onion_voxel_shared PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/version.hpp.in
    ${CMAKE_BINARY_DIR}/generated/version.hpp
    @ONLY
)
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/version.cpp.in
    ${CMAKE_BINARY_DIR}/generated/version.cpp
    @ONLY
)
target_sources(onion_voxel_shared PRIVATE ${CMAKE_BINARY_DIR}/generated/version.cpp)
target_include_directories(onion_voxel_shared PUBLIC ${CMAKE_BINARY_DIR}/generated)
