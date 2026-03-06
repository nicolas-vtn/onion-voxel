#pragma once

#include <memory>
#include <shared_mutex>
#include <thread>
#include <unordered_map>

#include <shared/thread_safe_queue/ThreadSafeQueue.hpp>
#include <shared/world/world_manager/WorldManager.hpp>

#include "../Variables.hpp"
#include "../camera/camera.hpp"
#include "../texture_atlas/TextureAtlas.hpp"
#include "chunk_mesh/ChunkMesh.hpp"
#include "chunk_mesh/MeshBuilder.hpp"

namespace onion::voxel
{
	class WorldRenderer
	{
		// ----- Constructor / Destructor -----
	  public:
		WorldRenderer(std::shared_ptr<WorldManager> worldManager, std::shared_ptr<Camera> camera);
		~WorldRenderer();

		// ----- Public API -----
	  public:
		void PrepareForRendering();
		void PrepareForRenderingOpaque();
		void PrepareForRenderingCutout();
		void PrepareForRenderingTransparent();
		void ResetOpenGLState();
		void Render();

		// ----- Getters / Setters -----
	  public:
		// ----- World Manager -----
	  private:
		std::shared_ptr<WorldManager> m_WorldManager;

		// ----- Events Handling -----
	  private:
		void SubscribeToWorldManagerEvents();
		std::vector<EventHandle> m_EventHandles;

		void Handle_ChunkAdded(const std::shared_ptr<Chunk>& chunk);
		void Handle_ChunkRemoved(const std::shared_ptr<Chunk>& chunk);

		// ----- Camera -----
	  private:
		std::shared_ptr<Camera> m_Camera;

		// ----- Chunk Meshes -----
	  private:
		mutable std::shared_mutex m_MutexChunkMeshes;
		std::unordered_map<glm::ivec2, std::shared_ptr<ChunkMesh>, IVec2Hash> m_ChunkMeshes;

		// ----- Texture Atlas and Block Registry -----
	  private:
		std::shared_ptr<TextureAtlas> m_TextureAtlas =
			std::make_shared<TextureAtlas>(GetMinecraftTexturesPath() / "block");

		// ----- Mesh Building -----
	  private:
		ThreadSafeQueue<std::shared_ptr<Chunk>> m_ChunksToBuildMeshFor;
		std::jthread m_ThreadMeshBuilder;
		void MeshBuilderThreadFunction(std::stop_token st);
		MeshBuilder m_MeshBuilder;
	};
} // namespace onion::voxel
