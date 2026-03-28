#pragma once

#include <deque>
#include <memory>
#include <string>

#include <onion/ThreadSafeQueue.hpp>

#include <renderer/OpenGL.hpp>

#include <renderer/EngineContext.hpp>
#include <renderer/camera/Camera.hpp>
#include <renderer/shader/shader.hpp>
#include <renderer/texture/texture.hpp>

#include <shared/entities/entity/player/Player.hpp>
#include <shared/entities/entity_manager/EntityManager.hpp>
#include <shared/http_file_downloader/HttpFileDownloader.hpp>

#include "Cuboid.hpp"
#include "TextureTileMapper.hpp"
#include "skeleton/SkeletonPlayer.hpp"

namespace onion::voxel
{

	class EntityRenderer
	{

		// ------- FORWARD DECLARATIONS -------
	  private:
		enum class SkinVersion : uint8_t
		{
			Legacy,
			Modern
		};

		// ------- CONSTRUCTOR & DESTRUCTOR -------
	  public:
		EntityRenderer(const std::shared_ptr<Camera>& camera);
		~EntityRenderer() = default;

		// ------- RENDERING -------
	  public:
		void RenderEntities(std::vector<std::string> HiddenEntities = std::vector<std::string>());

		void ReloadTextures();

		void Unload();

	  private:
		void Initialize();
		bool m_IsInitialized = false;

		// ------- RENDER ENTITIES -------
	  private:
		void BuildEntityMesh(const std::shared_ptr<Entity>& Entity);

		void BuildPlayerMesh(const std::shared_ptr<Player>& Player);

		void BuildPlayerMesh(const SkeletonPlayer& skeleton, EntityRenderer::SkinVersion skinVersion);
		void BuildPlayerMesh_Legacy(const SkeletonPlayer& skeleton);
		void BuildPlayerMesh_Modern(const SkeletonPlayer& skeleton);

		// ------- RENDER DATA -------
	  private:
		std::vector<float> m_VerticesEntities;

		// ------- PLAYERS SKINS MANAGEMENT -------
	  private:
		// Number of vertices for one player (6 cuboids * 6 faces * 2 triangles * 3 vertices)
		constexpr static int PLAYER_SKIN_VERTICES_COUNT = 6 * 6 * 2 * 3;

		mutable std::shared_mutex m_MutexPlayersSkinsIndex;
		std::unordered_map<size_t, std::string>
			m_PlayersSkinsIndex; // Maps one index in the m_VerticesEntities to one PlayerName

		mutable std::shared_mutex m_MutexPlayersSkins;
		std::unordered_map<std::string, Texture> m_PlayersSkins; // Maps one PlayerName to one Texture

		ThreadSafeQueue<Texture> m_TexturesToDelete;

		// ------- CAMERA -------
	  private:
		std::shared_ptr<Camera> m_Camera;

		// ------ ASYNC SKIN LOADER ------
	  private:
		void LoadPlayerSkinAsync(const std::string& playerName);
		void LoadPlayerSkin(const std::string& playerName);

		mutable std::mutex m_QueueMutex;
		std::deque<std::string> m_PlayersSkinsLoadingQueue;
		std::condition_variable_any m_QueueCv;

		std::jthread m_ThreadPlayerSkinsLoader;
		void Threaded_PlayerSkinsLoader(std::stop_token stoken);

		// ------- SKIN VERSION -------
	  private:
		SkinVersion GetPlayerSkinVersion(const std::string& playerName) const;

		// ------- SHADER -------
	  private:
		Shader m_ShaderEntity;

		// ------- TEXTURES -------
	  private:
		TextureTileMapper m_PlayerTextureTileMapper_Modern;
		TextureTileMapper m_PlayerTextureTileMapper_Legacy;

		static inline const std::filesystem::path m_DefaultPlayerTexturePath =
			std::filesystem::path("assets") / "minecraft" / "textures" / "entity" / "player" / "wide" / "steve.png";
		Texture m_DefaultPlayerTexture;

		void MapPlayerTexture();

		// ------- BUILD BUFFERS -------
	  private:
		void BuildBuffers(const std::shared_ptr<Player>& player);

		// ------- OPENGL BUFFERS -------
	  private:
		unsigned int m_VAO = 0;
		unsigned int m_VBO = 0;

		static constexpr int VERTEX_SIZE = 5; // Vertex size in floats (x, y, z, TexX, TexY)

		// ------- BUFFER BUILDER HELPERS -------
	  private:
		void AddCuboidWithTexture(const Cuboid& cuboid,
								  const TextureTile& front,
								  const TextureTile& back,
								  const TextureTile& top,
								  const TextureTile& bottom,
								  const TextureTile& left,
								  const TextureTile& right);
		void AddCuboidFace(const Cuboid::FacePositions& facePositions, const TextureTile& textureTile);

		// ------- DEBUG RENDERING -------
	  private:
		void RenderPlayerDebugPanel();

		bool m_RenderPlayerBoundingBoxes{true};
		void RenderPlayersBoundingBoxes();

		// ------- ENUMS -------
	  private:
		enum class PlayerTexturePart : int
		{
			HEAD_TOP,
			HEAD_BOTTOM,
			HEAD_FRONT,
			HEAD_BACK,
			HEAD_LEFT,
			HEAD_RIGHT,

			BODY_TOP,
			BODY_BOTTOM,
			BODY_FRONT,
			BODY_BACK,
			BODY_LEFT,
			BODY_RIGHT,

			LEFT_ARM_TOP,
			LEFT_ARM_BOTTOM,
			LEFT_ARM_FRONT,
			LEFT_ARM_BACK,
			LEFT_ARM_LEFT,
			LEFT_ARM_RIGHT,

			RIGHT_ARM_TOP,
			RIGHT_ARM_BOTTOM,
			RIGHT_ARM_FRONT,
			RIGHT_ARM_BACK,
			RIGHT_ARM_LEFT,
			RIGHT_ARM_RIGHT,

			LEFT_LEG_TOP,
			LEFT_LEG_BOTTOM,
			LEFT_LEG_FRONT,
			LEFT_LEG_BACK,
			LEFT_LEG_LEFT,
			LEFT_LEG_RIGHT,

			RIGHT_LEG_TOP,
			RIGHT_LEG_BOTTOM,
			RIGHT_LEG_FRONT,
			RIGHT_LEG_BACK,
			RIGHT_LEG_LEFT,
			RIGHT_LEG_RIGHT
		};
	};

} // namespace onion::voxel
