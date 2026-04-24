#pragma once

#include <memory>
#include <shared_mutex>
#include <unordered_map>

#include <onion/ThreadSafeQueue.hpp>

#include <shared/world/world_manager/WorldManager.hpp>

#include <renderer/camera/Camera.hpp>
#include <renderer/skybox/Skybox.hpp>
#include <renderer/texture_atlas/TextureAtlas.hpp>
#include <renderer/world_renderer/chunk_mesh/ChunkMesh.hpp>
#include <renderer/world_renderer/chunk_mesh/MeshBuilder.hpp>

namespace onion::voxel
{
	class WorldRenderer
	{
		// ----- Structs Declarations -----
	  public:
		struct FrustumPlane;

		// ----- Constructor / Destructor -----
	  public:
		WorldRenderer(std::shared_ptr<WorldManager> worldManager, std::shared_ptr<Camera> camera);
		~WorldRenderer();

		// ----- Public API -----
	  public:
		void Initialize();

		void PrepareForRendering();
		void PrepareForRenderingOpaque();
		void PrepareForRenderingCutout();
		void PrepareForRenderingTransparent();
		void ResetOpenGLState();
		void Render();

		void ReloadTextures();

		void MarkAllChunkMeshesDirty();

		void DeleteChunkMeshesAsync();
		void DeleteChunkMeshAsync(const glm::ivec2& chunkPos);

		void DeleteChunkMeshes();

		void Unload();
		static void StaticUnload();

		// ----- Getters / Setters -----
	  public:
		uint64_t GetVertexCount() const;
		uint64_t GetChunkMeshesCount() const;

		// ----- States -----
	  private:
		bool m_RenderChunkBorders{false};
		bool m_HasShaderBeenInitialized{false};

		// ----- World Manager -----
	  private:
		std::shared_ptr<WorldManager> m_WorldManager;

		// ----- Events Handling -----
	  private:
		void SubscribeToWorldManagerEvents();
		std::vector<EventHandle> m_EventHandles;

		void Handle_ChunkAdded(const std::shared_ptr<Chunk>& chunk);
		void Handle_ChunkRemoved(const std::shared_ptr<Chunk>& chunk);
		void Handle_BlocksChanged(const WorldManager::BlocksChangedEventArgs& args);

		// ----- Internal Helpers -----
	  private:
		void MarkNeighboringChunkMeshesDirty(const glm::ivec2& chunkPosition);
		void RenderChunkBorders();
		void InitializeShaderVariables();
		void MarkNeighboringSubChunkMeshesDirty(const glm::ivec3& blockPosition);

		bool IsChunkInFrustum(const glm::ivec2& chunkPos, int chunkHeight, const std::array<FrustumPlane, 6>& planes);

		// ----- Camera -----
	  private:
		std::shared_ptr<Camera> m_Camera;
		std::unordered_set<glm::ivec2> m_ChunksInFrustum;
		bool m_UseFrustumCulling = true;
		bool m_UpdateFrustumCulling = true;

		// ----- Skybox -----
	  private:
		Skybox m_Skybox;

		// ----- Chunk Meshes -----
	  private:
		mutable std::shared_mutex m_MutexChunkMeshes;
		std::unordered_map<glm::ivec2, std::shared_ptr<ChunkMesh>> m_ChunkMeshes;

		// ----- Texture Atlas and Block Registry -----
	  private:
		std::shared_ptr<TextureAtlas> m_TextureAtlas = std::make_shared<TextureAtlas>();

		// ----- Mesh Building -----
	  private:
		MeshBuilder m_MeshBuilder;
		void RebuildDirtyChunkMeshesAsync();

		// ----- Mesh Deletion and Cleanup -----
	  private:
		ThreadSafeQueue<std::shared_ptr<ChunkMesh>> m_ChunkMeshesToDelete;

		// ----- ImGui menu -----
	  private:
		void RenderDebugPanel();
		bool m_UseFaceCulling = true;

		// ----- Structs -----
	  public:
		struct FrustumPlane
		{
			glm::vec3 normal;
			float distance;

			void Normalize()
			{
				float length = glm::length(normal);
				normal /= length;
				distance /= length;
			}

			float GetSignedDistanceToPlane(const glm::vec3& point) const { return glm::dot(normal, point) + distance; }
		};
	};
} // namespace onion::voxel
