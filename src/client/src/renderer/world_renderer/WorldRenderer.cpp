#include "WorldRenderer.hpp"

#include <iostream>

namespace onion::voxel
{
	WorldRenderer::WorldRenderer(std::shared_ptr<WorldManager> worldManager, std::shared_ptr<Camera> camera)
		: m_WorldManager(worldManager), m_Camera(camera), m_MeshBuilder(worldManager, m_TextureAtlas)
	{
		SubscribeToWorldManagerEvents();

		m_ThreadMeshBuilder = std::jthread([this](std::stop_token st) { MeshBuilderThreadFunction(st); });
	};

	WorldRenderer::~WorldRenderer() {};

	void WorldRenderer::PrepareForRendering()
	{
		// Get Camera projection, view and ProjView Matix
		glm::mat4 projectionMatrix = m_Camera->GetProjectionMatrix();
		glm::mat4 viewMatrix = m_Camera->GetViewMatrix();
		glm::mat4 viewProjMatrix = projectionMatrix * viewMatrix;

		// Bind Texture Atlas
		//SubChunkMesh::s_TextureAtlas.Bind();
		m_TextureAtlas->Bind();

		// Sets Uniforms
		SubChunkMesh::s_Shader.Use();
		SubChunkMesh::s_Shader.setVec3("u_LightColor", 1.0f, 1.0f, 1.0f);
		SubChunkMesh::s_Shader.setBool("u_UseOcclusion", true);
		SubChunkMesh::s_Shader.setBool("u_UseFaceShading", true);
		SubChunkMesh::s_Shader.setMat4("u_ViewProjMatrix", viewProjMatrix);
		SubChunkMesh::s_Shader.setBool("u_RenderCutout", false);
	}

	void WorldRenderer::PrepareForRenderingOpaque()
	{
		// ----- Shader Setup -----
		SubChunkMesh::s_Shader.Use();
		SubChunkMesh::s_Shader.setBool("u_RenderCutout", false);

		// ----- OpenGL State Setup -----
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	void WorldRenderer::PrepareForRenderingCutout()
	{
		// ----- Shader Setup -----
		SubChunkMesh::s_Shader.Use();
		SubChunkMesh::s_Shader.setBool("u_RenderCutout", true);

		// ----- OpenGL State Setup -----
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	void WorldRenderer::PrepareForRenderingTransparent()
	{
		// ----- Shader Setup -----
		SubChunkMesh::s_Shader.Use();
		SubChunkMesh::s_Shader.setBool("u_RenderCutout", false);

		// ----- OpenGL State Setup -----
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	void WorldRenderer::ResetOpenGLState()
	{
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	void WorldRenderer::Render()
	{
		DeleteChunkMeshes();

		PrepareForRendering();

		PrepareForRenderingOpaque();

		// Render chunks
		std::shared_lock lock(m_MutexChunkMeshes);
		for (const auto& [chunkPos, chunkMesh] : m_ChunkMeshes)
		{
			chunkMesh->RenderOpaque();
		}

		PrepareForRenderingCutout();

		for (const auto& [chunkPos, chunkMesh] : m_ChunkMeshes)
		{
			chunkMesh->RenderCutout();
		}

		PrepareForRenderingTransparent();

		for (const auto& [chunkPos, chunkMesh] : m_ChunkMeshes)
		{
			chunkMesh->RenderTransparent();
		}

		ResetOpenGLState();
	}

	void WorldRenderer::DeleteAllChunkMeshes()
	{
		std::unique_lock lock(m_MutexChunkMeshes);
		for (const auto& [chunkPos, chunkMesh] : m_ChunkMeshes)
		{
			m_ChunkMeshesToDelete.Push(chunkMesh);
		}
		m_ChunkMeshes.clear();
	}

	void WorldRenderer::SubscribeToWorldManagerEvents()
	{
		m_EventHandles.push_back(m_WorldManager->ChunkAdded.Subscribe([this](const std::shared_ptr<Chunk>& chunk)
																	  { Handle_ChunkAdded(chunk); }));

		m_EventHandles.push_back(m_WorldManager->ChunkRemoved.Subscribe([this](const std::shared_ptr<Chunk>& chunk)
																		{ Handle_ChunkRemoved(chunk); }));
	}

	void WorldRenderer::Handle_ChunkAdded(const std::shared_ptr<Chunk>& chunk)
	{
		if (!chunk)
		{
			std::cerr << "Error: Received null chunk in Handle_ChunkAdded" << std::endl;
			return;
		}

		std::cout << "Chunk added at position: (" << chunk->GetPosition().x << ", " << chunk->GetPosition().y << ")"
				  << std::endl;

		// Create a new chunk mesh for the added chunk and add it to the chunk meshes map
		const glm::ivec2 chunkPos = chunk->GetPosition();
		std::shared_ptr<ChunkMesh> chunkMesh = std::make_shared<ChunkMesh>(chunkPos, chunk);
		{
			std::unique_lock lock(m_MutexChunkMeshes);

			// Check if a chunk mesh already exists for this chunk position
			auto it = m_ChunkMeshes.find(chunkPos);
			if (it != m_ChunkMeshes.end())
			{
				// If a chunk mesh already exists, mark it for deletion and remove it from the map
				m_ChunkMeshesToDelete.Push(it->second);
				m_ChunkMeshes.erase(it);
			}

			// Add the new chunk mesh to the map
			m_ChunkMeshes[chunkPos] = chunkMesh;
		}

		// Request the mesh builder to build the mesh for the new chunk mesh
		m_ChunkMeshesToRebuild.Push(chunkMesh);
	}

	void WorldRenderer::Handle_ChunkRemoved(const std::shared_ptr<Chunk>& chunk)
	{
		std::unique_lock lock(m_MutexChunkMeshes);

		const glm::ivec2 chunkPos = chunk->GetPosition();
		auto it = m_ChunkMeshes.find(chunkPos);
		if (it != m_ChunkMeshes.end())
		{
			m_ChunkMeshesToDelete.Push(it->second);
			m_ChunkMeshes.erase(it);
		}
	}

	void WorldRenderer::MeshBuilderThreadFunction(std::stop_token st)
	{
		while (!st.stop_requested())
		{
			std::shared_ptr<ChunkMesh> chunkMesh;

			if (m_ChunkMeshesToRebuild.TryPop(chunkMesh))
			{
				m_MeshBuilder.UpdateChunkMesh(chunkMesh);
			}
			else
			{
				// No chunks to build mesh for, sleep for a short time to avoid busy waiting
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		}
	}

	void WorldRenderer::DeleteChunkMeshes()
	{
		std::shared_ptr<ChunkMesh> chunkMesh;
		while (m_ChunkMeshesToDelete.TryPop(chunkMesh))
		{
			chunkMesh->Delete();
		}
	}

} // namespace onion::voxel
