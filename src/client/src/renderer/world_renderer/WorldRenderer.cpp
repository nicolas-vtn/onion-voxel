#include "WorldRenderer.hpp"

#include <imgui.h>

#include <iostream>

#include "../debug_draws/DebugDraws.hpp"
#include <shared/utils/Utils.hpp>

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
		m_TextureAtlas->Bind();

		// Sets Uniforms
		SubChunkMesh::s_Shader.Use();
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
		RenderDebugPanel();

		if (!m_HasShaderBeenInitialized)
		{
			InitializeShaderVariables();
		}

		if (m_RenderChunkBorders)
		{
			RenderChunkBorders();
		}

		RebuildDirtyChunkMeshesAsync();
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

	void WorldRenderer::DeleteChunkMeshesAsync()
	{
		std::unique_lock lock(m_MutexChunkMeshes);
		for (const auto& [chunkPos, chunkMesh] : m_ChunkMeshes)
		{
			m_ChunkMeshesToDelete.Push(chunkMesh);
		}
		m_ChunkMeshes.clear();
	}

	void WorldRenderer::DeleteChunkMeshAsync(const glm::ivec2& chunkPos)
	{
		std::unique_lock lock(m_MutexChunkMeshes);
		auto it = m_ChunkMeshes.find(chunkPos);
		if (it != m_ChunkMeshes.end())
		{
			m_ChunkMeshesToDelete.Push(it->second);
			m_ChunkMeshes.erase(it);
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
		if (!chunk)
		{
			std::cerr << "Error: Received null chunk in Handle_ChunkAdded" << std::endl;
			return;
		}

		//std::cout << "Chunk added at position: (" << chunk->GetPosition().x << ", " << chunk->GetPosition().y << ")"
		//		  << std::endl;

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

		// Mark neighboring chunk meshes as dirty so they will be rebuilt to update their faces that are adjacent to the new chunk
		MarkNeighboringChunkMeshesDirty(chunkPos);
	}

	void WorldRenderer::Handle_ChunkRemoved(const std::shared_ptr<Chunk>& chunk)
	{
		const glm::ivec2 chunkPos = chunk->GetPosition();

		DeleteChunkMeshAsync(chunkPos);

		// Mark neighboring chunk meshes as dirty so they will be rebuilt to update their faces that are adjacent to the removed chunk
		MarkNeighboringChunkMeshesDirty(chunkPos);
	}

	void WorldRenderer::MarkNeighboringChunkMeshesDirty(const glm::ivec2& chunkPosition)
	{
		std::shared_lock lock(m_MutexChunkMeshes);
		const std::array<glm::ivec2, 4> neighborOffsets = {
			glm::ivec2(-1, 0), glm::ivec2(1, 0), glm::ivec2(0, -1), glm::ivec2(0, 1)};

		for (const auto& offset : neighborOffsets)
		{
			const glm::ivec2 neighborPos = chunkPosition + offset;
			auto it = m_ChunkMeshes.find(neighborPos);
			if (it != m_ChunkMeshes.end())
			{
				it->second->SetAllSubChunkMeshesDirty(true);
			}
		}
	}

	inline std::pair<glm::vec3, glm::vec3> GetChunkCorners(const glm::ivec2& chunkPos, float maxY)
	{
		const float chunkSize = static_cast<float>(WorldConstants::SUBCHUNK_SIZE);
		const glm::vec3 worldPos = glm::vec3(chunkPos.x * chunkSize, 0.0f, chunkPos.y * chunkSize);
		return {worldPos, worldPos + glm::vec3(chunkSize, maxY, chunkSize)};
	}

	void WorldRenderer::RenderChunkBorders()
	{
		// ----- Colors and Line Widths -----
		constexpr glm::vec3 linesColor = glm::vec3(1.0f, 1.0f, 0.0f);

		constexpr int cornersLineWidth = 4;
		constexpr float cornersAlpha = 1.0f;

		constexpr int bordersLineWidth = 1;
		constexpr float bordersAlpha = 0.5f;

		// ----- Get Camera Chunk Corners -----
		const glm::vec3 cameraPos = m_Camera->GetPosition();
		const glm::ivec2 cameraChunkPos = Utils::WorldToChunkPosition(cameraPos);
		const auto cameraChunk = m_WorldManager->GetChunk(cameraChunkPos);

		if (!cameraChunk)
		{
			return;
		}

		const float maxY = cameraChunk->GetSubChunkCount() * WorldConstants::SUBCHUNK_SIZE;
		const auto [minCorner, maxCorner] = GetChunkCorners(cameraChunkPos, maxY);

		// Render Borders for the chunk of the camera
		DebugDraws::DrawWorldBoxMinMax(
			minCorner, maxCorner, glm::vec4(linesColor, cornersAlpha), cornersLineWidth, false);

		constexpr int spacing = 2;
		const float minY = minCorner.y;

		// ----- Borders along X (vary Z) -----
		for (int z = (int) minCorner.z; z <= (int) maxCorner.z; z += spacing)
		{
			// x = min
			DebugDraws::DrawWorldLine({minCorner.x, minY, (float) z},
									  {minCorner.x, maxY, (float) z},
									  glm::vec4(linesColor, bordersAlpha),
									  bordersLineWidth,
									  false);

			// x = max
			DebugDraws::DrawWorldLine({maxCorner.x, minY, (float) z},
									  {maxCorner.x, maxY, (float) z},
									  glm::vec4(linesColor, bordersAlpha),
									  bordersLineWidth,
									  false);
		}

		// ----- Borders along Z (vary X) -----
		for (int x = (int) minCorner.x; x <= (int) maxCorner.x; x += spacing)
		{
			// z = min
			DebugDraws::DrawWorldLine({(float) x, minY, minCorner.z},
									  {(float) x, maxY, minCorner.z},
									  glm::vec4(linesColor, bordersAlpha),
									  bordersLineWidth,
									  false);

			// z = max
			DebugDraws::DrawWorldLine({(float) x, minY, maxCorner.z},
									  {(float) x, maxY, maxCorner.z},
									  glm::vec4(linesColor, bordersAlpha),
									  bordersLineWidth,
									  false);
		}

		// ----- SubChunk Borders -----
		constexpr glm::vec3 subChunkLinesColor = glm::vec3(1.0f, 0.0f, 1.0f);
		constexpr int subChunkLineWidth = 1;
		if (cameraPos.y > 0.0f && cameraPos.y < maxY)
		{
			const int subChunkIndex = static_cast<int>(cameraPos.y) / WorldConstants::SUBCHUNK_SIZE;
			const float y = (float) subChunkIndex * WorldConstants::SUBCHUNK_SIZE;
			DebugDraws::DrawWorldBoxMinMax({minCorner.x, y, minCorner.z},
										   {maxCorner.x, y + WorldConstants::SUBCHUNK_SIZE, maxCorner.z},
										   glm::vec4(subChunkLinesColor, bordersAlpha),
										   subChunkLineWidth,
										   false);
		}
	}

	void WorldRenderer::InitializeShaderVariables()
	{
		SubChunkMesh::s_Shader.Use();
		SubChunkMesh::s_Shader.setInt("u_TextureAtlas", 0);

		SubChunkMesh::SetLightColor(glm::vec3(1.0f, 1.0f, 1.0f));
		SubChunkMesh::SetUseFaceShading(true);
		SubChunkMesh::SetUseOcclusion(true);

		m_HasShaderBeenInitialized = true;
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

	void WorldRenderer::RebuildDirtyChunkMeshesAsync()
	{
		std::shared_lock lock(m_MutexChunkMeshes);
		for (const auto& [chunkPos, chunkMesh] : m_ChunkMeshes)
		{
			if (chunkMesh->IsDirty())
			{
				chunkMesh->StartRebuilding();
				m_ChunkMeshesToRebuild.Push(chunkMesh);
			}
		}
	}

	inline std::string FormatThousands(uint64_t value)
	{
		std::string s = std::to_string(value);

		int insertPosition = s.length() - 3;
		while (insertPosition > 0)
		{
			s.insert(insertPosition, " ");
			insertPosition -= 3;
		}

		return s;
	}

	void WorldRenderer::RenderDebugPanel()
	{
		ImGui::Begin("World Renderer");

		ImGui::Text("Chunk Meshes: %s", FormatThousands(GetChunkMeshesCount()).c_str());
		ImGui::Text("Vertices: %s", FormatThousands(GetVertexCount()).c_str());
		ImGui::Checkbox("Render Chunk Borders", &m_RenderChunkBorders);

		// ---- SubChunk Shader Configuration ----
		ImGui::Separator();
		bool useFaceShading = SubChunkMesh::GetUseFaceShading();
		if (ImGui::Checkbox("Use Face Shading", &useFaceShading))
		{
			SubChunkMesh::SetUseFaceShading(useFaceShading);
		}

		bool useOcclusion = SubChunkMesh::GetUseOcclusion();
		if (ImGui::Checkbox("Use Occlusion", &useOcclusion))
		{
			SubChunkMesh::SetUseOcclusion(useOcclusion);
		}

		glm::vec3 lightColor = SubChunkMesh::GetLightColor();
		float color[3] = {lightColor.r, lightColor.g, lightColor.b};
		if (ImGui::ColorEdit3("Light Color", color))
		{
			SubChunkMesh::SetLightColor(glm::vec3(color[0], color[1], color[2]));
		}

		ImGui::End();
	}

	void WorldRenderer::DeleteChunkMeshes()
	{
		std::shared_ptr<ChunkMesh> chunkMesh;
		while (m_ChunkMeshesToDelete.TryPop(chunkMesh))
		{
			chunkMesh->Delete();
		}
	}

	uint64_t WorldRenderer::GetVertexCount() const
	{
		std::shared_lock lock(m_MutexChunkMeshes);
		uint64_t vertexCount = 0;
		for (const auto& [chunkPos, chunkMesh] : m_ChunkMeshes)
		{
			if (chunkMesh)
			{
				vertexCount += chunkMesh->GetVertexCount();
			}
		}
		return vertexCount;
	}

	uint64_t WorldRenderer::GetChunkMeshesCount() const
	{
		std::shared_lock lock(m_MutexChunkMeshes);
		return m_ChunkMeshes.size();
	}

} // namespace onion::voxel
