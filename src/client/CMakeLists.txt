cmake_minimum_required(VERSION 3.20)

project(onion_voxel_client LANGUAGES CXX)

# Copy assets to build directory
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/../../assets" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

set(ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../assets/app_icons/Vox_Client_Title.ico")

configure_file(
    app_icon.rc.in
    ${CMAKE_CURRENT_BINARY_DIR}/app_icon.rc
    @ONLY
)

add_executable(OnionVoxel

    "main.cpp"
    "src/Client.cpp"

	"src/network_client/NetworkClient.cpp"

	"src/renderer/Renderer.cpp"

	"src/renderer/texture/Texture.cpp"
	"src/renderer/texture/stb_image.cpp"

	"src/renderer/shader/shader.cpp"

	"src/renderer/inputs_manager/inputs.hpp"
	"src/renderer/inputs_manager/inputs_manager.cpp"

	"src/renderer/gui/Gui.cpp"
	"src/renderer/gui/GuiElement.cpp"

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

	"src/renderer/gui/panels/demo_panel/DemoPanel.cpp"
	"src/renderer/gui/panels/demo_scrolling_panel/DemoScrollingPanel.cpp"
	"src/renderer/gui/panels/main_menu_panel/MainMenuPanel.cpp"
	"src/renderer/gui/panels/pause_panel/PausePanel.cpp"
	"src/renderer/gui/panels/options_panel/OptionsPanel.cpp"
	"src/renderer/gui/panels/resource_packs_panel/ResourcePacksPanel.cpp"
	"src/renderer/gui/panels/resource_packs_panel/ResourcePackTile.cpp"
	"src/renderer/gui/panels/singleplayer_panel/SingleplayerPanel.cpp"
	"src/renderer/gui/panels/singleplayer_panel/WorldTile.cpp"
	"src/renderer/gui/panels/video_settings_panel/VideoSettingsPanel.cpp"
	"src/renderer/gui/panels/controls_panel/ControlsPanel.cpp"
	"src/renderer/gui/panels/mouse_settings_panel/MouseSettingsPanel.cpp"
    "src/renderer/gui/panels/key_binds_panel/KeyBindsPanel.cpp"
	"src/renderer/gui/panels/key_binds_panel/KeyBindsTile.cpp"

	"src/renderer/world_renderer/WorldRenderer.cpp"
	"src/renderer/camera/Camera.cpp"
	"src/renderer/world_renderer/chunk_mesh/ChunkMesh.cpp"
	"src/renderer/world_renderer/chunk_mesh/SubChunkMesh.cpp"

	"src/renderer/texture_atlas/TextureAtlas.cpp"
	"src/renderer/world_renderer/block_registry/BlockRegistry.cpp"

	"src/renderer/world_renderer/chunk_mesh/MeshBuilder.cpp"

	"src/renderer/debug_draws/DebugDraws.cpp"

	"src/renderer/skybox/Skybox.cpp"

	"src/renderer/assets_manager/AssetsManager.cpp"
	"src/renderer/assets_manager/resource_pack_manager/ResourcePackManager.cpp"

	"src/renderer/key_binds/KeyBinds.cpp"

	"src/renderer/entity_renderer/EntityRenderer.cpp"
	"src/renderer/entity_renderer/Cuboid.cpp"
	"src/renderer/entity_renderer/skeleton/SkeletonPlayer.cpp"
	"src/renderer/entity_renderer/skeleton/Skeleton.cpp"
	"src/renderer/entity_renderer/TextureTileMapper.cpp"

	"src/user_settings/UserSettings.cpp"

 )

if (WIN32)
    target_sources(OnionVoxel PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/app_icon.rc)
endif()

target_link_libraries(OnionVoxel
    PUBLIC
        glfw
        glad
        glm
        stb
		nlohmann_json::nlohmann_json
		imgui::imgui
		enet::enet
		winmm
		ws2_32

		onion::event
		onion::logger
		onion::timer
		onion::threadsafequeue
		onion::threadpool

        onion_voxel_server
		onion_voxel_shared
)

target_compile_options(OnionVoxel PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>
)

target_compile_features(OnionVoxel PRIVATE cxx_std_20)

target_compile_options(OnionVoxel
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)

target_include_directories(OnionVoxel
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
