cmake_minimum_required(VERSION 3.20)

project(onion_voxel_client LANGUAGES CXX)

# Copy assets to build directory
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/../../assets" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

add_executable(OnionVoxel
    "main.cpp"
    "src/Client.cpp"

	"src/renderer/Renderer.cpp"

	"src/renderer/texture/Texture.cpp"
	"src/renderer/texture/stb_image.cpp"

	"src/renderer/shader/shader.cpp"

	"src/renderer/inputs_manager/inputs.hpp"
	"src/renderer/inputs_manager/inputs_manager.cpp"

	"src/renderer/gui/GuiElement.cpp"
	"src/renderer/gui/controls/button/Button.cpp"
	"src/renderer/gui/layouts/demo_panel/DemoPanel.cpp"

	"src/renderer/gui/font/Font.cpp"
	"src/renderer/gui/controls/sprite/Sprite.cpp"
	"src/renderer/gui/nine_slice_sprite/NineSliceSprite.cpp"

)

target_link_libraries(OnionVoxel
    PUBLIC
        glfw
        glad
        glm
        stb
		nlohmann_json::nlohmann_json
		imgui::imgui
		onion_event
    PRIVATE
        onion_voxel_server
)

target_compile_features(OnionVoxel PRIVATE cxx_std_20)

target_include_directories(OnionVoxel
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
