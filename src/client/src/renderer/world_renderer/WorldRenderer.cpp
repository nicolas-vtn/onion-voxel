#include "WorldRenderer.hpp"

#include <iostream>

namespace onion::voxel
{
	WorldRenderer::WorldRenderer(std::shared_ptr<WorldManager> worldManager, std::shared_ptr<Camera> camera)
		: m_WorldManager(worldManager), m_Camera(camera)
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
		SubChunkMesh::s_TextureAtlas.Bind();

		// Sets Uniforms
		SubChunkMesh::s_Shader.Use();
		SubChunkMesh::s_Shader.setVec3("u_LightColor", 1.0f, 1.0f, 1.0f);
		SubChunkMesh::s_Shader.setBool("u_UseOcclusion", true);
		SubChunkMesh::s_Shader.setBool("u_UseFaceShading", true);
		SubChunkMesh::s_Shader.setMat4("u_ViewProjMatrix", viewProjMatrix);
	}

	void WorldRenderer::Render()
	{
		PrepareForRendering();

		// Render chunks
		std::shared_lock lock(m_MutexChunkMeshes);
		for (const auto& [chunkPos, chunkMesh] : m_ChunkMeshes)
		{
			chunkMesh->RenderOpaque();
		}
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
		std::cout << "Chunk added at position: (" << chunk->GetPosition().x << ", " << chunk->GetPosition().y << ")"
				  << std::endl;

		// Adds the chunk to the m_ChunksToBuildMeshFor queue to be processed by the mesh builder thread
		m_ChunksToBuildMeshFor.Push(chunk);
	}

	void WorldRenderer::Handle_ChunkRemoved(const std::shared_ptr<Chunk>& chunk)
	{
		std::unique_lock lock(m_MutexChunkMeshes);

		const glm::ivec2 chunkPos = chunk->GetPosition();
		auto it = m_ChunkMeshes.find(chunkPos);
		if (it != m_ChunkMeshes.end())
		{
			// Marks the chunk mesh as DeleteRequested.
			it->second->SetDeleteRequested(true);
		}
	}

	void WorldRenderer::MeshBuilderThreadFunction(std::stop_token st)
	{
		while (!st.stop_requested())
		{
			std::shared_ptr<Chunk> chunk;

			if (m_ChunksToBuildMeshFor.TryPop(chunk))
			{
				// Build mesh for chunk
				glm::ivec2 chunkPos = chunk->GetPosition();
				std::cout << "Building mesh for chunk at position: (" << chunkPos.x << ", " << chunkPos.y << ")"
						  << std::endl;

				std::shared_ptr<ChunkMesh> chunkMesh = std::make_shared<ChunkMesh>(chunk);

				// Adds the ChunkMesh to the m_ChunkMeshes map
				{
					std::unique_lock lock(m_MutexChunkMeshes);
					m_ChunkMeshes[chunkPos] = chunkMesh;
				}
			}
			else
			{
				// No chunks to build mesh for, sleep for a short time to avoid busy waiting
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		}
	}

} // namespace onion::voxel
