# Minimum CMake version requirement
cmake_minimum_required(VERSION 3.20)

# Project declaration and language setup
project(onion_voxel_client LANGUAGES CXX)

# Path to application icon
set(ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../assets/app_icons/Vox_Client_Title.ico")

# Generate Windows resource file from template
configure_file(
    app_icon.rc.in
    ${CMAKE_CURRENT_BINARY_DIR}/app_icon.rc
    @ONLY
)

# Define executable and all source files
add_executable(OnionVoxel

    "main.cpp"
    "src/Client.cpp"

	"src/network_client/NetworkClient.cpp"

	"src/renderer/Renderer.cpp"

	"src/renderer/texture/texture.cpp"
	"src/renderer/texture/stb_image.cpp"

	"src/renderer/shader/shader.cpp"

	"src/renderer/inputs_manager/inputs.hpp"
	"src/renderer/inputs_manager/inputs_manager.cpp"

	"src/renderer/gui/Gui.cpp"
	"src/renderer/gui/GuiElement.cpp"

	"src/renderer/gui/ui_block_mesh/UiBlockMesh.cpp"

	"src/renderer/gui/nine_slice_sprite/NineSliceSprite.cpp"
	"src/renderer/gui/font/Font.cpp"
	"src/renderer/gui/colored_background/ColoredBackground.cpp"

	"src/renderer/gui/controls/button/Button.cpp"
	"src/renderer/gui/controls/sprite/Sprite.cpp"
	"src/renderer/gui/controls/label/Label.cpp"
	"src/renderer/gui/controls/checkbox/Checkbox.cpp"
	"src/renderer/gui/controls/text_field/TextField.cpp"
	"src/renderer/gui/controls/slider/Slider.cpp"
	"src/renderer/gui/controls/scroller/Scroller.cpp"
	"src/renderer/gui/controls/tooltip/Tooltip.cpp"

	"src/renderer/gui/panels/demo_panel/DemoPanel.cpp"
	"src/renderer/gui/panels/demo_scrolling_panel/DemoScrollingPanel.cpp"
	"src/renderer/gui/panels/demo_texts_panel/DemoTextsPanel.cpp"
	"src/renderer/gui/panels/main_menu_panel/MainMenuPanel.cpp"
	"src/renderer/gui/panels/pause_panel/PausePanel.cpp"
	"src/renderer/gui/panels/options_panel/OptionsPanel.cpp"
	"src/renderer/gui/panels/resource_packs_panel/ResourcePacksPanel.cpp"
	"src/renderer/gui/panels/resource_packs_panel/ResourcePackTile.cpp"
	"src/renderer/gui/panels/singleplayer_panel/SingleplayerPanel.cpp"
	"src/renderer/gui/panels/singleplayer_panel/WorldTile.cpp"
	"src/renderer/gui/panels/multiplayer_panel/MultiplayerPanel.cpp"
	"src/renderer/gui/panels/multiplayer_panel/ServerTile.cpp"
	"src/renderer/gui/panels/video_settings_panel/VideoSettingsPanel.cpp"
	"src/renderer/gui/panels/controls_panel/ControlsPanel.cpp"
	"src/renderer/gui/panels/mouse_settings_panel/MouseSettingsPanel.cpp"
    "src/renderer/gui/panels/key_binds_panel/KeyBindsPanel.cpp"
	"src/renderer/gui/panels/key_binds_panel/KeyBindsTile.cpp"
	"src/renderer/gui/panels/hud_panel/HudPanel.cpp"
	"src/renderer/gui/panels/inventory_panel/InventoryPanel.cpp"

	"src/renderer/world_renderer/WorldRenderer.cpp"
	"src/renderer/camera/Camera.cpp"
	"src/renderer/world_renderer/chunk_mesh/ChunkMesh.cpp"
	"src/renderer/world_renderer/chunk_mesh/SubChunkMesh.cpp"

	"src/renderer/texture_atlas/TextureAtlas.cpp"
	"src/renderer/world_renderer/block_registry/BlockRegistry.cpp"
	"../shared/shared/world/block/BlockModel.cpp"
	

	"src/renderer/world_renderer/chunk_mesh/MeshBuilder.cpp"

	"src/renderer/debug_draws/DebugDraws.cpp"

	"src/renderer/skybox/Skybox.cpp"

	"src/renderer/assets_manager/AssetsManager.cpp"
	"src/renderer/assets_manager/resource_pack_manager/ResourcePackManager.cpp"

	"src/renderer/key_binds/KeyBinds.cpp"

	"src/renderer/entity_renderer/EntityRenderer.cpp"
	"src/renderer/entity_renderer/cuboid.cpp"
	"src/renderer/entity_renderer/skeleton/SkeletonPlayer.cpp"
	"src/renderer/entity_renderer/skeleton/skeleton.cpp"
	"src/renderer/entity_renderer/TextureTileMapper.cpp"

	"src/user_settings/UserSettings.cpp"

)

# Add Windows-specific resource file (icon)
if (WIN32)
    target_sources(OnionVoxel PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/app_icon.rc)
endif()

# Link external and internal libraries
target_link_libraries(OnionVoxel
    PUBLIC
        glfw
        glad
        glm
        stb
		nlohmann_json::nlohmann_json
		imgui::imgui
		enet::enet
		cereal::cereal

		onion::event
		onion::logger
		onion::timer
		onion::threadsafequeue
		onion::threadpool

        onion_voxel_server
		onion_voxel_shared
)

# Link additional Windows system libraries
if (WIN32)
    target_link_libraries(OnionVoxel
        PUBLIC
            winmm
            ws2_32
    )
endif()

# Link CURL on Linux systems
if (UNIX AND NOT APPLE)
    find_package(CURL REQUIRED)

    target_link_libraries(onion_voxel_shared
        PUBLIC
            CURL::libcurl
    )
endif()

# Ensure correct __cplusplus macro behavior on MSVC
target_compile_options(OnionVoxel PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>
)

# Require C++20 standard
target_compile_features(OnionVoxel PRIVATE cxx_std_20)

# Enable compiler warnings
target_compile_options(OnionVoxel
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)

# Add include directories
target_include_directories(OnionVoxel
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Install executable to client folder
install(TARGETS OnionVoxel
    RUNTIME DESTINATION client
)

# Install assets directory
install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets/"
    DESTINATION client/assets
)
