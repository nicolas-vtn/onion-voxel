#include "EntityRenderer.hpp"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <chrono>
#include <iostream>

#include <renderer/Variables.hpp>
#include <renderer/debug_draws/DebugDraws.hpp>

namespace onion::voxel
{
	EntityRenderer::EntityRenderer(const std::shared_ptr<Camera>& camera)
		: m_Camera(camera),
		  m_ShaderEntity(GetAssetsPath() / "shaders" / "entity.vert", GetAssetsPath() / "shaders" / "entity.frag")
	{

		// Starts the thread that will download the player skins asynchronously
		m_ThreadPlayerSkinsLoader =
			std::jthread([this](std::stop_token stoken) { Threaded_PlayerSkinsLoader(stoken); });
	}

	void EntityRenderer::Initialize()
	{
		// Load textures
		ReloadTextures();

		// Setup Shader
		m_ShaderEntity.Use();
		m_ShaderEntity.setInt("atlas", 0);

		MapPlayerTexture();

		// Generate VAO and VBO
		glGenVertexArrays(1, &m_VAO);
		glGenBuffers(1, &m_VBO);

		constexpr GLint stride = VERTEX_SIZE * sizeof(float);

		// Sets the layout
		glBindVertexArray(m_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

		// position: 3 floats at offset 0
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*) 0);
		glEnableVertexAttribArray(0);

		// texcoord: 2 floats right after position (offset 3 floats)
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*) (3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		m_IsInitialized = true;
	}

	void EntityRenderer::RenderEntities(std::vector<std::string> HiddenEntities)
	{
		if (!m_IsInitialized)
		{
			Initialize();
		}

		// Render DEBUG
		RenderPlayerDebugPanel();
		if (m_RenderPlayerBoundingBoxes)
		{
			RenderPlayersBoundingBoxes();
		}

		// Delete the textures that are in the deletion queue
		Texture tex;
		while (m_TexturesToDelete.TryPop(tex))
		{
			tex.Delete();
		}

		const auto entities = EngineContext::Get().World->Entities->GetAllPlayers();

		// Clear the vertices after rendering
		m_VerticesEntities.clear();

		// Clear the players skins index
		{
			std::unique_lock<std::shared_mutex> lock(m_MutexPlayersSkinsIndex);
			m_PlayersSkinsIndex.clear();
		}

		// Iterate through all entities and build their meshes
		for (const auto& [uuid, entity] : entities)
		{
			if (!entity)
				continue;

			// If the entity is in the hidden list, skip it
			if (std::find(HiddenEntities.begin(), HiddenEntities.end(), uuid) != HiddenEntities.end())
				continue;

			// Build the mesh for the entity
			BuildEntityMesh(entity);
		}

		if (m_VerticesEntities.empty())
			return;

		// Use Shader, set uniforms and bind textures
		const glm::mat4 view = m_Camera->GetViewMatrix();
		const glm::mat4 proj = m_Camera->GetProjectionMatrix();
		m_ShaderEntity.Use();
		m_ShaderEntity.setMat4("view", view);
		m_ShaderEntity.setMat4("projection", proj);
		m_DefaultPlayerTexture.Bind();

		// Sends the vertices to the GPU
		glBindVertexArray(m_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
		glBufferData(
			GL_ARRAY_BUFFER, m_VerticesEntities.size() * sizeof(float), m_VerticesEntities.data(), GL_STREAM_DRAW);

		// Draw the Players
		{
			std::shared_lock<std::shared_mutex> lock(m_MutexPlayersSkinsIndex);
			std::shared_lock<std::shared_mutex> lockSkins(m_MutexPlayersSkins);

			for (const auto& [index, playerName] : m_PlayersSkinsIndex)
			{

				// Check if the player skin is already loaded
				if (m_PlayersSkins.find(playerName) != m_PlayersSkins.end())
				{
					// Bind the player's skin texture
					m_PlayersSkins[playerName].Bind();
				}
				else
				{
					// Bind the default player texture
					m_DefaultPlayerTexture.Bind();
					// Load the player's skin texture from the URL (ASYNC)
					LoadPlayerSkinAsync(playerName);
				}

				// Draw the player
				glDrawArrays(GL_TRIANGLES, static_cast<GLint>(index), PLAYER_SKIN_VERTICES_COUNT);
			}
		}

		glBindVertexArray(0);
	}

	void EntityRenderer::ReloadTextures()
	{
		m_DefaultPlayerTexture.Delete();

		auto defaultPlayerTextureData =
			EngineContext::Get().Assets->GetResourcePackFileBinary(m_DefaultPlayerTexturePath);

		m_DefaultPlayerTexture = Texture("DefaultPlayerTexture", defaultPlayerTextureData, true);
	}

	void EntityRenderer::Unload()
	{
		glDeleteVertexArrays(1, &m_VAO);
		glDeleteBuffers(1, &m_VBO);

		m_ShaderEntity.Delete();
		m_DefaultPlayerTexture.Delete();

		{
			std::unique_lock<std::shared_mutex> lock(m_MutexPlayersSkins);
			for (auto& [playerName, texture] : m_PlayersSkins)
			{
				texture.Delete();
			}
			m_PlayersSkins.clear();
		}
	}

	void EntityRenderer::BuildEntityMesh(const std::shared_ptr<Entity>& Entity)
	{
		if (!Entity)
			return;

		// Render the entity based on its type
		switch (Entity->Type)
		{

			case EntityType::Player:
				{
					auto player = std::dynamic_pointer_cast<Player>(Entity);
					if (player)
					{
						BuildPlayerMesh(player);
					}
					break;
				}

				// TODO: Add cases for other entity types (mobs, NPCs, items, etc.)

			default:
				break;
		}
	}

	static inline float fractional_seconds_now()
	{
		using namespace std::chrono;

		const auto now = steady_clock::now();
		const auto since_epoch = now.time_since_epoch();
		const auto whole_secs = duration_cast<seconds>(since_epoch);
		const duration<float> frac = since_epoch - whole_secs; // remainder as seconds
		return frac.count();								   // in [0,1)
	}

	void EntityRenderer::BuildPlayerMesh(const std::shared_ptr<Player>& Player)
	{
		if (!Player)
			return;

		// Add the entry to the players skins index
		{
			std::unique_lock<std::shared_mutex> lock(m_MutexPlayersSkinsIndex);
			m_PlayersSkinsIndex[m_VerticesEntities.size() / VERTEX_SIZE] = Player->GetName();
		}

		// Retrieves the Skin Version
		const SkinVersion skinVersion = GetPlayerSkinVersion(Player->GetName());

		const float scale = (1 / 18.f);
		// const float scale = (1.f);

		SkeletonPlayer skeleton(Player->GetPosition(), Player->GetFacing(), scale);

		// Gets the decimal part of seconds
		const float speed = 2.0f;
		const float progress = glm::fract(fractional_seconds_now() * speed);

		skeleton.SetState(Player->GetState(), progress);

		BuildPlayerMesh(skeleton, skinVersion);
	}

	void EntityRenderer::MapPlayerTexture()
	{

		// ------------------------------------------------- MODERN SKIN -------------------------------------------------
		// ------------------------------------------------- MODERN SKIN -------------------------------------------------
		{
			m_PlayerTextureTileMapper_Modern.SetTextureDimensions(64, 64);
			m_PlayerTextureTileMapper_Modern.SetFlipY(true); // Flip Y axis for OpenGL

			// -------- HEAD -------
			const glm::ivec2 HeadTop_BottomLeft{8, 8};
			const glm::ivec2 HeadTop_TopRight{16, 0};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::HEAD_TOP), HeadTop_BottomLeft, HeadTop_TopRight);

			const glm::ivec2 HeadBottom_BottomLeft{16, 8};
			const glm::ivec2 HeadBottom_TopRight{24, 0};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::HEAD_BOTTOM), HeadBottom_BottomLeft, HeadBottom_TopRight);

			const glm::ivec2 HeadFront_BottomLeft{8, 16};
			const glm::ivec2 HeadFront_TopRight{16, 8};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::HEAD_FRONT), HeadFront_BottomLeft, HeadFront_TopRight);

			const glm::ivec2 HeadBack_BottomLeft{24, 16};
			const glm::ivec2 HeadBack_TopRight{32, 8};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::HEAD_BACK), HeadBack_BottomLeft, HeadBack_TopRight);

			const glm::ivec2 HeadLeft_BottomLeft{0, 16};
			const glm::ivec2 HeadLeft_TopRight{8, 8};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::HEAD_LEFT), HeadLeft_BottomLeft, HeadLeft_TopRight);

			const glm::ivec2 HeadRight_BottomLeft{16, 16};
			const glm::ivec2 HeadRight_TopRight{24, 8};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::HEAD_RIGHT), HeadRight_BottomLeft, HeadRight_TopRight);

			// -------- BODY -------
			const glm::ivec2 BodyTop_BottomLeft{20, 20};
			const glm::ivec2 BodyTop_TopRight{28, 16};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::BODY_TOP), BodyTop_BottomLeft, BodyTop_TopRight);

			const glm::ivec2 BodyBottom_BottomLeft{28, 20};
			const glm::ivec2 BodyBottom_TopRight{36, 16};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::BODY_BOTTOM), BodyBottom_BottomLeft, BodyBottom_TopRight);

			const glm::ivec2 BodyFront_BottomLeft{20, 32};
			const glm::ivec2 BodyFront_TopRight{28, 20};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::BODY_FRONT), BodyFront_BottomLeft, BodyFront_TopRight);

			const glm::ivec2 BodyBack_BottomLeft{32, 32};
			const glm::ivec2 BodyBack_TopRight{40, 20};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::BODY_BACK), BodyBack_BottomLeft, BodyBack_TopRight);

			const glm::ivec2 BodyLeft_BottomLeft{16, 32};
			const glm::ivec2 BodyLeft_TopRight{20, 20};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::BODY_LEFT), BodyLeft_BottomLeft, BodyLeft_TopRight);

			const glm::ivec2 BodyRight_BottomLeft{28, 32};
			const glm::ivec2 BodyRight_TopRight{32, 20};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::BODY_RIGHT), BodyRight_BottomLeft, BodyRight_TopRight);

			// -------- LEFT ARM -------
			const glm::ivec2 ArmLeftTop_BottomLeft{44, 20};
			const glm::ivec2 ArmLeftTop_TopRight{48, 16};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::LEFT_ARM_TOP), ArmLeftTop_BottomLeft, ArmLeftTop_TopRight);

			const glm::ivec2 ArmLeftBottom_BottomLeft{48, 20};
			const glm::ivec2 ArmLeftBottom_TopRight{52, 16};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::LEFT_ARM_BOTTOM), ArmLeftBottom_BottomLeft, ArmLeftBottom_TopRight);

			const glm::ivec2 ArmLeftFront_BottomLeft{44, 32};
			const glm::ivec2 ArmLeftFront_TopRight{48, 20};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::LEFT_ARM_FRONT), ArmLeftFront_BottomLeft, ArmLeftFront_TopRight);

			const glm::ivec2 ArmLeftBack_BottomLeft{52, 32};
			const glm::ivec2 ArmLeftBack_TopRight{56, 20};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::LEFT_ARM_BACK), ArmLeftBack_BottomLeft, ArmLeftBack_TopRight);

			const glm::ivec2 ArmLeftLeft_BottomLeft{40, 32};
			const glm::ivec2 ArmLeftLeft_TopRight{44, 20};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::LEFT_ARM_LEFT), ArmLeftLeft_BottomLeft, ArmLeftLeft_TopRight);

			const glm::ivec2 ArmLeftRight_BottomLeft{48, 32};
			const glm::ivec2 ArmLeftRight_TopRight{52, 20};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::LEFT_ARM_RIGHT), ArmLeftRight_BottomLeft, ArmLeftRight_TopRight);

			// -------- RIGHT ARM -------
			const glm::ivec2 ArmRightTop_BottomLeft{36, 52};
			const glm::ivec2 ArmRightTop_TopRight{40, 48};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::RIGHT_ARM_TOP), ArmRightTop_BottomLeft, ArmRightTop_TopRight);

			const glm::ivec2 ArmRightBottom_BottomLeft{40, 52};
			const glm::ivec2 ArmRightBottom_TopRight{44, 48};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(static_cast<int>(PlayerTexturePart::RIGHT_ARM_BOTTOM),
															ArmRightBottom_BottomLeft,
															ArmRightBottom_TopRight);

			const glm::ivec2 ArmRightFront_BottomLeft{36, 64};
			const glm::ivec2 ArmRightFront_TopRight{40, 52};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::RIGHT_ARM_FRONT), ArmRightFront_BottomLeft, ArmRightFront_TopRight);

			const glm::ivec2 ArmRightBack_BottomLeft{44, 64};
			const glm::ivec2 ArmRightBack_TopRight{48, 52};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::RIGHT_ARM_BACK), ArmRightBack_BottomLeft, ArmRightBack_TopRight);

			const glm::ivec2 ArmRightLeft_BottomLeft{32, 64};
			const glm::ivec2 ArmRightLeft_TopRight{36, 52};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::RIGHT_ARM_LEFT), ArmRightLeft_BottomLeft, ArmRightLeft_TopRight);

			const glm::ivec2 ArmRightRight_BottomLeft{40, 64};
			const glm::ivec2 ArmRightRight_TopRight{44, 52};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::RIGHT_ARM_RIGHT), ArmRightRight_BottomLeft, ArmRightRight_TopRight);

			// -------- LEFT LEG -------
			const glm::ivec2 LegLeftTop_BottomLeft{20, 52};
			const glm::ivec2 LegLeftTop_TopRight{24, 48};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::LEFT_LEG_TOP), LegLeftTop_BottomLeft, LegLeftTop_TopRight);

			const glm::ivec2 LegLeftBottom_BottomLeft{24, 52};
			const glm::ivec2 LegLeftBottom_TopRight{28, 48};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::LEFT_LEG_BOTTOM), LegLeftBottom_BottomLeft, LegLeftBottom_TopRight);

			const glm::ivec2 LegLeftFront_BottomLeft{20, 64};
			const glm::ivec2 LegLeftFront_TopRight{24, 52};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::LEFT_LEG_FRONT), LegLeftFront_BottomLeft, LegLeftFront_TopRight);

			const glm::ivec2 LegLeftBack_BottomLeft{28, 63};
			const glm::ivec2 LegLeftBack_TopRight{32, 52};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::LEFT_LEG_BACK), LegLeftBack_BottomLeft, LegLeftBack_TopRight);

			const glm::ivec2 LegLeftLeft_BottomLeft{24, 64};
			const glm::ivec2 LegLeftLeft_TopRight{28, 52};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::LEFT_LEG_LEFT), LegLeftLeft_BottomLeft, LegLeftLeft_TopRight);

			const glm::ivec2 LegLeftRight_BottomLeft{16, 64};
			const glm::ivec2 LegLeftRight_TopRight{20, 52};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::LEFT_LEG_RIGHT), LegLeftRight_BottomLeft, LegLeftRight_TopRight);

			// -------- RIGHT LEG -------
			const glm::ivec2 LegRightTop_BottomLeft{4, 20};
			const glm::ivec2 LegRightTop_TopRight{8, 16};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::RIGHT_LEG_TOP), LegRightTop_BottomLeft, LegRightTop_TopRight);

			const glm::ivec2 LegRightBottom_BottomLeft{8, 20};
			const glm::ivec2 LegRightBottom_TopRight{12, 16};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(static_cast<int>(PlayerTexturePart::RIGHT_LEG_BOTTOM),
															LegRightBottom_BottomLeft,
															LegRightBottom_TopRight);

			const glm::ivec2 LegRightFront_BottomLeft{4, 32};
			const glm::ivec2 LegRightFront_TopRight{8, 20};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::RIGHT_LEG_FRONT), LegRightFront_BottomLeft, LegRightFront_TopRight);

			const glm::ivec2 LegRightBack_BottomLeft{12, 32};
			const glm::ivec2 LegRightBack_TopRight{16, 20};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::RIGHT_LEG_BACK), LegRightBack_BottomLeft, LegRightBack_TopRight);

			const glm::ivec2 LegRightLeft_BottomLeft{8, 32};
			const glm::ivec2 LegRightLeft_TopRight{12, 20};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::RIGHT_LEG_LEFT), LegRightLeft_BottomLeft, LegRightLeft_TopRight);

			const glm::ivec2 LegRightRight_BottomLeft{0, 32};
			const glm::ivec2 LegRightRight_TopRight{4, 20};
			m_PlayerTextureTileMapper_Modern.AddTextureTile(
				static_cast<int>(PlayerTexturePart::RIGHT_LEG_RIGHT), LegRightRight_BottomLeft, LegRightRight_TopRight);
		}

		// ------------------------------------------------- LEGACY SKIN -------------------------------------------------
		// ------------------------------------------------- LEGACY SKIN -------------------------------------------------
		{
			m_PlayerTextureTileMapper_Legacy.SetTextureDimensions(64, 32);
			m_PlayerTextureTileMapper_Legacy.SetFlipY(true); // Flip Y axis for OpenGL

			// -------- HEAD -------
			const glm::ivec2 HeadTop_BottomLeft{8, 8};
			const glm::ivec2 HeadTop_TopRight{16, 0};
			m_PlayerTextureTileMapper_Legacy.AddTextureTile(
				static_cast<int>(PlayerTexturePart::HEAD_TOP), HeadTop_BottomLeft, HeadTop_TopRight);

			const glm::ivec2 HeadBottom_BottomLeft{16, 8};
			const glm::ivec2 HeadBottom_TopRight{24, 0};
			m_PlayerTextureTileMapper_Legacy.AddTextureTile(
				static_cast<int>(PlayerTexturePart::HEAD_BOTTOM), HeadBottom_BottomLeft, HeadBottom_TopRight);

			const glm::ivec2 HeadFront_BottomLeft{8, 16};
			const glm::ivec2 HeadFront_TopRight{16, 8};
			m_PlayerTextureTileMapper_Legacy.AddTextureTile(
				static_cast<int>(PlayerTexturePart::HEAD_FRONT), HeadFront_BottomLeft, HeadFront_TopRight);

			const glm::ivec2 HeadBack_BottomLeft{24, 16};
			const glm::ivec2 HeadBack_TopRight{32, 8};
			m_PlayerTextureTileMapper_Legacy.AddTextureTile(
				static_cast<int>(PlayerTexturePart::HEAD_BACK), HeadBack_BottomLeft, HeadBack_TopRight);

			const glm::ivec2 HeadLeft_BottomLeft{0, 16};
			const glm::ivec2 HeadLeft_TopRight{8, 8};
			m_PlayerTextureTileMapper_Legacy.AddTextureTile(
				static_cast<int>(PlayerTexturePart::HEAD_LEFT), HeadLeft_BottomLeft, HeadLeft_TopRight);

			const glm::ivec2 HeadRight_BottomLeft{16, 16};
			const glm::ivec2 HeadRight_TopRight{24, 8};
			m_PlayerTextureTileMapper_Legacy.AddTextureTile(
				static_cast<int>(PlayerTexturePart::HEAD_RIGHT), HeadRight_BottomLeft, HeadRight_TopRight);

			// -------- BODY -------
			const glm::ivec2 BodyTop_BottomLeft{20, 20};
			const glm::ivec2 BodyTop_TopRight{28, 16};
			m_PlayerTextureTileMapper_Legacy.AddTextureTile(
				static_cast<int>(PlayerTexturePart::BODY_TOP), BodyTop_BottomLeft, BodyTop_TopRight);

			const glm::ivec2 BodyBottom_BottomLeft{28, 20};
			const glm::ivec2 BodyBottom_TopRight{36, 16};
			m_PlayerTextureTileMapper_Legacy.AddTextureTile(
				static_cast<int>(PlayerTexturePart::BODY_BOTTOM), BodyBottom_BottomLeft, BodyBottom_TopRight);

			const glm::ivec2 BodyFront_BottomLeft{20, 32};
			const glm::ivec2 BodyFront_TopRight{28, 20};
			m_PlayerTextureTileMapper_Legacy.AddTextureTile(
				static_cast<int>(PlayerTexturePart::BODY_FRONT), BodyFront_BottomLeft, BodyFront_TopRight);

			const glm::ivec2 BodyBack_BottomLeft{32, 32};
			const glm::ivec2 BodyBack_TopRight{40, 20};
			m_PlayerTextureTileMapper_Legacy.AddTextureTile(
				static_cast<int>(PlayerTexturePart::BODY_BACK), BodyBack_BottomLeft, BodyBack_TopRight);

			const glm::ivec2 BodyLeft_BottomLeft{16, 32};
			const glm::ivec2 BodyLeft_TopRight{20, 20};
			m_PlayerTextureTileMapper_Legacy.AddTextureTile(
				static_cast<int>(PlayerTexturePart::BODY_LEFT), BodyLeft_BottomLeft, BodyLeft_TopRight);

			const glm::ivec2 BodyRight_BottomLeft{28, 32};
			const glm::ivec2 BodyRight_TopRight{32, 20};
			m_PlayerTextureTileMapper_Legacy.AddTextureTile(
				static_cast<int>(PlayerTexturePart::BODY_RIGHT), BodyRight_BottomLeft, BodyRight_TopRight);

			// -------- LEFT ARM -------
			const glm::ivec2 ArmLeftTop_BottomLeft{44, 20};
			const glm::ivec2 ArmLeftTop_TopRight{48, 16};
			m_PlayerTextureTileMapper_Legacy.AddTextureTile(
				static_cast<int>(PlayerTexturePart::LEFT_ARM_TOP), ArmLeftTop_BottomLeft, ArmLeftTop_TopRight);

			const glm::ivec2 ArmLeftBottom_BottomLeft{48, 20};
			const glm::ivec2 ArmLeftBottom_TopRight{52, 16};
			m_PlayerTextureTileMapper_Legacy.AddTextureTile(
				static_cast<int>(PlayerTexturePart::LEFT_ARM_BOTTOM), ArmLeftBottom_BottomLeft, ArmLeftBottom_TopRight);

			const glm::ivec2 ArmLeftFront_BottomLeft{44, 32};
			const glm::ivec2 ArmLeftFront_TopRight{48, 20};
			m_PlayerTextureTileMapper_Legacy.AddTextureTile(
				static_cast<int>(PlayerTexturePart::LEFT_ARM_FRONT), ArmLeftFront_BottomLeft, ArmLeftFront_TopRight);

			const glm::ivec2 ArmLeftBack_BottomLeft{52, 32};
			const glm::ivec2 ArmLeftBack_TopRight{56, 20};
			m_PlayerTextureTileMapper_Legacy.AddTextureTile(
				static_cast<int>(PlayerTexturePart::LEFT_ARM_BACK), ArmLeftBack_BottomLeft, ArmLeftBack_TopRight);

			const glm::ivec2 ArmLeftLeft_BottomLeft{40, 32};
			const glm::ivec2 ArmLeftLeft_TopRight{44, 20};
			m_PlayerTextureTileMapper_Legacy.AddTextureTile(
				static_cast<int>(PlayerTexturePart::LEFT_ARM_LEFT), ArmLeftLeft_BottomLeft, ArmLeftLeft_TopRight);

			const glm::ivec2 ArmLeftRight_BottomLeft{48, 32};
			const glm::ivec2 ArmLeftRight_TopRight{52, 20};
			m_PlayerTextureTileMapper_Legacy.AddTextureTile(
				static_cast<int>(PlayerTexturePart::LEFT_ARM_RIGHT), ArmLeftRight_BottomLeft, ArmLeftRight_TopRight);

			// -------- RIGHT ARM -------
			m_PlayerTextureTileMapper_Legacy.AddTextureTile(
				static_cast<int>(PlayerTexturePart::RIGHT_ARM_TOP), ArmLeftTop_BottomLeft, ArmLeftTop_TopRight);

			m_PlayerTextureTileMapper_Legacy.AddTextureTile(static_cast<int>(PlayerTexturePart::RIGHT_ARM_BOTTOM),
															ArmLeftBottom_BottomLeft,
															ArmLeftBottom_TopRight);

			m_PlayerTextureTileMapper_Legacy.AddTextureTile(
				static_cast<int>(PlayerTexturePart::RIGHT_ARM_FRONT), ArmLeftFront_BottomLeft, ArmLeftFront_TopRight);

			m_PlayerTextureTileMapper_Legacy.AddTextureTile(
				static_cast<int>(PlayerTexturePart::RIGHT_ARM_BACK), ArmLeftBack_BottomLeft, ArmLeftBack_TopRight);

			m_PlayerTextureTileMapper_Legacy.AddTextureTile(
				static_cast<int>(PlayerTexturePart::RIGHT_ARM_RIGHT), ArmLeftLeft_BottomLeft, ArmLeftLeft_TopRight);

			m_PlayerTextureTileMapper_Legacy.AddTextureTile(
				static_cast<int>(PlayerTexturePart::RIGHT_ARM_LEFT), ArmLeftRight_BottomLeft, ArmLeftRight_TopRight);

			// -------- RIGHT LEG -------
			const glm::ivec2 LegRightTop_BottomLeft{4, 20};
			const glm::ivec2 LegRightTop_TopRight{8, 16};
			m_PlayerTextureTileMapper_Legacy.AddTextureTile(
				static_cast<int>(PlayerTexturePart::RIGHT_LEG_TOP), LegRightTop_BottomLeft, LegRightTop_TopRight);

			const glm::ivec2 LegRightBottom_BottomLeft{8, 20};
			const glm::ivec2 LegRightBottom_TopRight{12, 16};
			m_PlayerTextureTileMapper_Legacy.AddTextureTile(static_cast<int>(PlayerTexturePart::RIGHT_LEG_BOTTOM),
															LegRightBottom_BottomLeft,
															LegRightBottom_TopRight);

			const glm::ivec2 LegRightFront_BottomLeft{4, 32};
			const glm::ivec2 LegRightFront_TopRight{8, 20};
			m_PlayerTextureTileMapper_Legacy.AddTextureTile(
				static_cast<int>(PlayerTexturePart::RIGHT_LEG_FRONT), LegRightFront_BottomLeft, LegRightFront_TopRight);

			const glm::ivec2 LegRightBack_BottomLeft{12, 32};
			const glm::ivec2 LegRightBack_TopRight{16, 20};
			m_PlayerTextureTileMapper_Legacy.AddTextureTile(
				static_cast<int>(PlayerTexturePart::RIGHT_LEG_BACK), LegRightBack_BottomLeft, LegRightBack_TopRight);

			const glm::ivec2 LegRightLeft_BottomLeft{8, 32};
			const glm::ivec2 LegRightLeft_TopRight{12, 20};
			m_PlayerTextureTileMapper_Legacy.AddTextureTile(
				static_cast<int>(PlayerTexturePart::RIGHT_LEG_RIGHT), LegRightLeft_BottomLeft, LegRightLeft_TopRight);

			const glm::ivec2 LegRightRight_BottomLeft{0, 32};
			const glm::ivec2 LegRightRight_TopRight{4, 20};
			m_PlayerTextureTileMapper_Legacy.AddTextureTile(
				static_cast<int>(PlayerTexturePart::RIGHT_LEG_LEFT), LegRightRight_BottomLeft, LegRightRight_TopRight);

			// -------- LEFT LEG -------
			m_PlayerTextureTileMapper_Legacy.AddTextureTile(
				static_cast<int>(PlayerTexturePart::LEFT_LEG_TOP), LegRightTop_BottomLeft, LegRightTop_TopRight);

			m_PlayerTextureTileMapper_Legacy.AddTextureTile(static_cast<int>(PlayerTexturePart::LEFT_LEG_BOTTOM),
															LegRightBottom_BottomLeft,
															LegRightBottom_TopRight);
			auto a = LegRightFront_BottomLeft;
			auto b = LegRightFront_TopRight;

			a.x = LegRightFront_TopRight.x;
			b.x = LegRightFront_BottomLeft.x;
			m_PlayerTextureTileMapper_Legacy.AddTextureTile(static_cast<int>(PlayerTexturePart::LEFT_LEG_FRONT), a, b);

			m_PlayerTextureTileMapper_Legacy.AddTextureTile(
				static_cast<int>(PlayerTexturePart::LEFT_LEG_BACK), LegRightBack_BottomLeft, LegRightBack_TopRight);

			m_PlayerTextureTileMapper_Legacy.AddTextureTile(
				static_cast<int>(PlayerTexturePart::LEFT_LEG_LEFT), LegRightLeft_BottomLeft, LegRightLeft_TopRight);

			auto c = LegRightRight_BottomLeft;
			auto d = LegRightRight_TopRight;
			c.x = LegRightRight_TopRight.x;
			d.x = LegRightRight_BottomLeft.x;
			m_PlayerTextureTileMapper_Legacy.AddTextureTile(static_cast<int>(PlayerTexturePart::LEFT_LEG_RIGHT), c, d);
		}
	}

	void EntityRenderer::AddCuboidWithTexture(const Cuboid& cuboid,
											  const TextureTile& front,
											  const TextureTile& back,
											  const TextureTile& top,
											  const TextureTile& bottom,
											  const TextureTile& left,
											  const TextureTile& right)
	{

		AddCuboidFace(cuboid.GetFrontFacePositions(), front);	// Front face
		AddCuboidFace(cuboid.GetBackFacePositions(), back);		// Back face
		AddCuboidFace(cuboid.GetTopFacePositions(), top);		// Top face
		AddCuboidFace(cuboid.GetBottomFacePositions(), bottom); // Bottom face
		AddCuboidFace(cuboid.GetLeftFacePositions(), left);		// Left face
		AddCuboidFace(cuboid.GetRightFacePositions(), right);	// Right face
	}

	void EntityRenderer::AddCuboidFace(const Cuboid::FacePositions& facePositions, const TextureTile& textureTile)
	{
		std::vector<float> tmpVertices{
			facePositions.BottomLeft.x,	 facePositions.BottomLeft.y,  facePositions.BottomLeft.z,
			textureTile.BottomLeft.x,	 textureTile.BottomLeft.y,	  facePositions.BottomRight.x,
			facePositions.BottomRight.y, facePositions.BottomRight.z, textureTile.BottomRight.x,
			textureTile.BottomRight.y,	 facePositions.TopRight.x,	  facePositions.TopRight.y,
			facePositions.TopRight.z,	 textureTile.TopRight.x,	  textureTile.TopRight.y,
			facePositions.TopRight.x,	 facePositions.TopRight.y,	  facePositions.TopRight.z,
			textureTile.TopRight.x,		 textureTile.TopRight.y,	  facePositions.BottomLeft.x,
			facePositions.BottomLeft.y,	 facePositions.BottomLeft.z,  textureTile.BottomLeft.x,
			textureTile.BottomLeft.y,	 facePositions.TopLeft.x,	  facePositions.TopLeft.y,
			facePositions.TopLeft.z,	 textureTile.TopLeft.x,		  textureTile.TopLeft.y,
		};
		m_VerticesEntities.insert(m_VerticesEntities.end(), tmpVertices.begin(), tmpVertices.end());
	}

	void EntityRenderer::RenderPlayerDebugPanel()
	{
		std::shared_ptr<Player> player = EngineContext::Get().GetLocalPlayer();

		ImGui::Begin("Player Debug");

		// Global Options
		ImGui::Text("Global Options");

		ImGui::Checkbox("Render Player Boxes", &m_RenderPlayerBoundingBoxes);

		ImGui::Separator();

		if (player)
		{
			std::string name = player->GetName();
			if (ImGui::InputText("Name", &name))
			{
				player->SetName(name);
			}

			if (ImGui::CollapsingHeader("Player Transform"))
			{
				if (player->HasTransform())
				{
					Transform transform = player->GetTransform();
					if (ImGui::InputFloat3("Position", &transform.Position.x))
					{
						player->SetTransform(transform);
					}
					if (ImGui::InputFloat3("Rotation", &transform.Rotation.x))
					{
						player->SetTransform(transform);
					}
					if (ImGui::InputFloat3("Scale", &transform.Scale.x))
					{
						player->SetTransform(transform);
					}
				}
				else
				{
					ImGui::Text("Player has no transform component");
				}
			}

			if (ImGui::CollapsingHeader("Player Physics Body"))
			{
				if (player->HasPhysicsBody())
				{
					PhysicsBody physicsBody = player->GetPhysicsBody();
					if (ImGui::InputFloat3("Velocity", &physicsBody.Velocity.x))
					{
						player->SetPhysicsBody(physicsBody);
					}
					if (ImGui::Checkbox("On Ground", &physicsBody.OnGround))
					{
						player->SetPhysicsBody(physicsBody);
					}
					if (ImGui::Checkbox("Is Flying", &physicsBody.IsFlying))
					{
						physicsBody.Velocity = glm::vec3(0.f);
						player->SetPhysicsBody(physicsBody);
					}
					if (ImGui::InputFloat("Mass", &physicsBody.Mass))
					{
						player->SetPhysicsBody(physicsBody);
					}
				}
				else
				{
					ImGui::Text("Player has no physics body component");
				}
			}
		}
		else
		{
			ImGui::Text("No local player");
		}

		ImGui::End();
	}

	void EntityRenderer::RenderPlayersBoundingBoxes()
	{
		const auto& players = EngineContext::Get().World->Entities->GetAllPlayers();

		for (const auto& [uuid, player] : players)
		{
			const glm::vec3 playerPos = player->GetPosition();
			const glm::vec3 centerPos = playerPos + glm::vec3(0.f, Player::Size.y * 0.5f, 0.f);
			const glm::vec3 boxColor = glm::vec3(1.0f, 0.0f, 0.0f);
			DebugDraws::DrawWorldBoxCenterSize(centerPos, Player::Size, glm::vec4(boxColor, 1.0f), 2, false);

			// Draw a line indicating the player's facing direction
			const glm::vec3 forward = player->GetFacing();
			const glm::vec3 playerEyePos = player->GetEyePosition();
			const glm::vec3 lineColor = glm::vec3(0.0f, 1.0f, 0.0f);
			DebugDraws::DrawWorldLine(playerEyePos, playerEyePos + forward, glm::vec4(lineColor, 1.0f), 2, false);

			// Draw a cross indicating the player's exact position
			const float crossSize = 0.1f;
			const glm::vec3 crossColor = glm::vec3(0.0f, 0.0f, 1.0f);
			DebugDraws::DrawWorldLine(playerPos - glm::vec3(crossSize, 0.0f, 0.0f),
									  playerPos + glm::vec3(crossSize, 0.0f, 0.0f),
									  glm::vec4(crossColor, 1.0f),
									  2,
									  false);

			DebugDraws::DrawWorldLine(playerPos - glm::vec3(0.0f, 0.0f, crossSize),
									  playerPos + glm::vec3(0.0f, 0.0f, crossSize),
									  glm::vec4(crossColor, 1.0f),
									  2,
									  false);
		}
	}

	void EntityRenderer::LoadPlayerSkinAsync(const std::string& playerName)
	{

		// Check if the player is already in the loading queue
		{
			std::unique_lock<std::mutex> lock(m_QueueMutex);
			if (std::find(m_PlayersSkinsLoadingQueue.begin(), m_PlayersSkinsLoadingQueue.end(), playerName) !=
				m_PlayersSkinsLoadingQueue.end())
			{
				// Already in the loading queue
				return;
			}
		}

		// Add the player to the loading queue
		{
			std::lock_guard lk(m_QueueMutex);
			m_PlayersSkinsLoadingQueue.emplace_back(playerName);
		}
		m_QueueCv.notify_one();
	}

	void EntityRenderer::LoadPlayerSkin(const std::string& playerName)
	{

		if (playerName.empty())
			return;

		// Load the player's skin from the URL
		const std::string url = "https://minecraft.tools/download-skin/" + playerName;

		// Download the skin
		std::vector<uint8_t> skinData = HttpFileDownloader::DownloadFile(url);

		if (!skinData.empty())
		{
			Texture playerSkin("Player_" + playerName, skinData, true);
			std::unique_lock<std::shared_mutex> lock(m_MutexPlayersSkins);

			// If the skin was already loaded, move the previous to delete queue, and replace it with the new one
			if (auto it = m_PlayersSkins.find(playerName); it != m_PlayersSkins.end())
			{
				m_TexturesToDelete.Push(std::move(it->second));
				m_PlayersSkins.erase(it);
			}

			m_PlayersSkins[playerName] = std::move(playerSkin);
		}
	}

	void EntityRenderer::Threaded_PlayerSkinsLoader(std::stop_token stoken)
	{

		// Ensure we wake if stop is requested while waiting
		std::stop_callback on_stop(stoken, [this] { m_QueueCv.notify_all(); });

		for (;;)
		{
			std::string playerName;

			{
				std::unique_lock lk(m_QueueMutex);

				// Wait until there's work, or stop requested
				bool ready = m_QueueCv.wait(lk, stoken, [this] { return !m_PlayersSkinsLoadingQueue.empty(); });
				if (!ready)
				{
					// Woke due to stop request (or spurious + stop); time to exit
					break;
				}

				// Retrieve playerName from the queue (without popping yet)
				playerName = m_PlayersSkinsLoadingQueue.front();
			}

			// Do the work without holding the lock
			LoadPlayerSkin(playerName);

			// Work done; now pop the queue entry
			{
				std::lock_guard lk(m_QueueMutex);
				if (!m_PlayersSkinsLoadingQueue.empty() && m_PlayersSkinsLoadingQueue.front() == playerName)
				{
					m_PlayersSkinsLoadingQueue.pop_front();
				}
			}
		}
	}

	EntityRenderer::SkinVersion EntityRenderer::GetPlayerSkinVersion(const std::string& playerName) const
	{

		SkinVersion skinVersion;

		std::shared_lock<std::shared_mutex> lock(m_MutexPlayersSkins);

		// Check if the player's skin is loaded
		if (m_PlayersSkins.find(playerName) != m_PlayersSkins.end())
		{

			// Player skin is loaded, check its dimensions
			const auto& playerTexture = m_PlayersSkins.at(playerName);
			if (playerTexture.Width() == 64 && playerTexture.Height() == 64)
			{
				// Skin is in the new format (64x64)
				skinVersion = SkinVersion::Modern;
			}
			else
			{
				// Skin is in the old format (64x32)
				skinVersion = SkinVersion::Legacy;
			}
		}
		else
		{
			// Skin is not loaded yet, assume modern format
			skinVersion = SkinVersion::Modern;
		}

		return skinVersion;
	}

	void EntityRenderer::BuildPlayerMesh(const SkeletonPlayer& skeleton, EntityRenderer::SkinVersion skinVersion)
	{
		switch (skinVersion)
		{
			case EntityRenderer::SkinVersion::Legacy:
				BuildPlayerMesh_Legacy(skeleton);
				break;

			case EntityRenderer::SkinVersion::Modern:
				BuildPlayerMesh_Modern(skeleton);
				break;

			default:
				break;
		}
	}

	void EntityRenderer::BuildPlayerMesh_Legacy(const SkeletonPlayer& skeleton)
	{
		AddCuboidWithTexture(
			skeleton.Head,
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::HEAD_FRONT)),  // Front
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::HEAD_BACK)),   // Back
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::HEAD_TOP)),	   // Top
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::HEAD_BOTTOM)), // Bottom
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::HEAD_LEFT)),   // Left
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::HEAD_RIGHT))   // Right
		);

		AddCuboidWithTexture(
			skeleton.Body,
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::BODY_FRONT)),  // Front
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::BODY_BACK)),   // Back
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::BODY_TOP)),	   // Top
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::BODY_BOTTOM)), // Bottom
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::BODY_LEFT)),   // Left
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::BODY_RIGHT))   // Right
		);

		AddCuboidWithTexture(
			skeleton.LeftArm,
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::LEFT_ARM_FRONT)),
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::LEFT_ARM_BACK)),
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::LEFT_ARM_TOP)),
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::LEFT_ARM_BOTTOM)),
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::LEFT_ARM_LEFT)),
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::LEFT_ARM_RIGHT)));

		AddCuboidWithTexture(
			skeleton.RightArm,
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::RIGHT_ARM_FRONT)),
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::RIGHT_ARM_BACK)),
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::RIGHT_ARM_TOP)),
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::RIGHT_ARM_BOTTOM)),
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::RIGHT_ARM_LEFT)),
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::RIGHT_ARM_RIGHT)));

		AddCuboidWithTexture(
			skeleton.LeftLeg,
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::LEFT_LEG_FRONT)),
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::LEFT_LEG_BACK)),
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::LEFT_LEG_TOP)),
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::LEFT_LEG_BOTTOM)),
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::LEFT_LEG_LEFT)),
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::LEFT_LEG_RIGHT)));

		AddCuboidWithTexture(
			skeleton.RightLeg,
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::RIGHT_LEG_FRONT)),
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::RIGHT_LEG_BACK)),
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::RIGHT_LEG_TOP)),
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::RIGHT_LEG_BOTTOM)),
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::RIGHT_LEG_LEFT)),
			m_PlayerTextureTileMapper_Legacy.GetTextureTile(static_cast<int>(PlayerTexturePart::RIGHT_LEG_RIGHT)));
	}

	void EntityRenderer::BuildPlayerMesh_Modern(const SkeletonPlayer& skeleton)
	{
		AddCuboidWithTexture(
			skeleton.Head,
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::HEAD_FRONT)),  // Front
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::HEAD_BACK)),   // Back
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::HEAD_TOP)),	   // Top
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::HEAD_BOTTOM)), // Bottom
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::HEAD_LEFT)),   // Left
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::HEAD_RIGHT))   // Right
		);

		AddCuboidWithTexture(
			skeleton.Body,
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::BODY_FRONT)),  // Front
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::BODY_BACK)),   // Back
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::BODY_TOP)),	   // Top
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::BODY_BOTTOM)), // Bottom
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::BODY_LEFT)),   // Left
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::BODY_RIGHT))   // Right
		);

		AddCuboidWithTexture(
			skeleton.LeftArm,
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::LEFT_ARM_FRONT)),
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::LEFT_ARM_BACK)),
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::LEFT_ARM_TOP)),
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::LEFT_ARM_BOTTOM)),
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::LEFT_ARM_LEFT)),
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::LEFT_ARM_RIGHT)));

		AddCuboidWithTexture(
			skeleton.RightArm,
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::RIGHT_ARM_FRONT)),
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::RIGHT_ARM_BACK)),
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::RIGHT_ARM_TOP)),
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::RIGHT_ARM_BOTTOM)),
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::RIGHT_ARM_LEFT)),
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::RIGHT_ARM_RIGHT)));

		AddCuboidWithTexture(
			skeleton.LeftLeg,
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::LEFT_LEG_FRONT)),
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::LEFT_LEG_BACK)),
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::LEFT_LEG_TOP)),
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::LEFT_LEG_BOTTOM)),
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::LEFT_LEG_LEFT)),
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::LEFT_LEG_RIGHT)));

		AddCuboidWithTexture(
			skeleton.RightLeg,
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::RIGHT_LEG_FRONT)),
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::RIGHT_LEG_BACK)),
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::RIGHT_LEG_TOP)),
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::RIGHT_LEG_BOTTOM)),
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::RIGHT_LEG_LEFT)),
			m_PlayerTextureTileMapper_Modern.GetTextureTile(static_cast<int>(PlayerTexturePart::RIGHT_LEG_RIGHT)));
	}
} // namespace onion::voxel
