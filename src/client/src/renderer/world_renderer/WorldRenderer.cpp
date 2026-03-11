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
	};

	WorldRenderer::~WorldRenderer()
	{
		m_EventHandles.clear();
	};

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
		//SubChunkMesh::s_Shader.setMat4("u_ViewProjMatrix", viewProjMatrix);
		SubChunkMesh::s_Shader.setMat4("u_ViewProjMatrix", m_Camera->GetUntranslatedViewProjectionMatrix());
		SubChunkMesh::s_Shader.setVec3("u_CameraPosition", m_Camera->GetPosition());
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
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(-1.0f, -1.0f);
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
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(-1.0f, -1.0f);
	}

	void WorldRenderer::ResetOpenGLState()
	{
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
		glDisable(GL_POLYGON_OFFSET_FILL);
	}

	void WorldRenderer::Render()
	{
		// Matrix calculations
		const glm::mat4 view = m_Camera->GetViewMatrix();
		const glm::mat4 projection = m_Camera->GetProjectionMatrix();

		// Render SkyBox
		m_Skybox.Render(view, projection);

		// Render Debug Panel
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

		std::unordered_map<glm::ivec2, std::shared_ptr<ChunkMesh>, IVec2Hash> chunkMeshesSnapshot;
		{
			std::shared_lock lock(m_MutexChunkMeshes);
			chunkMeshesSnapshot = m_ChunkMeshes;
		}

		for (const auto& [chunkPos, chunkMesh] : chunkMeshesSnapshot)
		{
			chunkMesh->RenderOpaque();
		}

		PrepareForRenderingCutout();

		for (const auto& [chunkPos, chunkMesh] : chunkMeshesSnapshot)
		{
			chunkMesh->RenderCutout();
		}

		PrepareForRenderingTransparent();

		for (const auto& [chunkPos, chunkMesh] : chunkMeshesSnapshot)
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

		m_EventHandles.push_back(m_WorldManager->BlocksChanged.Subscribe(
			[this](const std::vector<std::pair<glm::ivec3, Block>>& blocks) { Handle_BlocksChanged(blocks); }));
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

		// If a chunk mesh already exists, just update it
		const glm::ivec2 chunkPos = chunk->GetPosition();
		std::shared_ptr<ChunkMesh> chunkMesh;
		{
			std::unique_lock lock(m_MutexChunkMeshes);
			auto it = m_ChunkMeshes.find(chunkPos);
			if (it != m_ChunkMeshes.end())
			{
				chunkMesh = it->second;
				chunkMesh->ChangeChunk(chunk);
			}
			else
			{
				chunkMesh = std::make_shared<ChunkMesh>(chunkPos, chunk);
				m_ChunkMeshes.emplace(chunkPos, chunkMesh);
			}
		}

		// Request the mesh builder to build the mesh for the new chunk mesh
		if (chunkMesh)
			m_MeshBuilder.UpdateChunkMeshAsync(chunkMesh);

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

	void WorldRenderer::Handle_BlocksChanged(const std::vector<std::pair<glm::ivec3, Block>>& blocks)
	{
		for (const auto& [worldPos, block] : blocks)
		{
			MarkNeighboringSubChunkMeshesDirty(worldPos);
		}
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
		const float chunkSize = static_cast<float>(WorldConstants::CHUNK_SIZE);
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

		const float maxY = cameraChunk->GetSubChunkCount() * WorldConstants::CHUNK_SIZE;
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
			const int subChunkIndex = static_cast<int>(cameraPos.y) / WorldConstants::CHUNK_SIZE;
			const float y = (float) subChunkIndex * WorldConstants::CHUNK_SIZE;
			DebugDraws::DrawWorldBoxMinMax({minCorner.x, y, minCorner.z},
										   {maxCorner.x, y + WorldConstants::CHUNK_SIZE, maxCorner.z},
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

	void WorldRenderer::MarkNeighboringSubChunkMeshesDirty(const glm::ivec3& blockPosition)
	{
		const glm::ivec2 chunkPos = Utils::WorldToChunkPosition(blockPosition);

		std::unordered_map<glm::ivec2, std::vector<uint8_t>, IVec2Hash> subChunkIndicesToMarkDirty;

		// Mark the subchunk mesh of the chunk that contains the block dirty
		uint8_t subChunkIndex = blockPosition.y / WorldConstants::CHUNK_SIZE;
		subChunkIndicesToMarkDirty[chunkPos].push_back(subChunkIndex);

		// Mark the neighboring subchunk mesh dirty if the block is on the edge of the chunk
		const glm::ivec3 localPos = Utils::WorldToLocalPosition(blockPosition);

		if (localPos.x == 0)
		{
			subChunkIndicesToMarkDirty[chunkPos + glm::ivec2(-1, 0)].push_back(subChunkIndex);
		}
		else if (localPos.x == WorldConstants::CHUNK_SIZE - 1)
		{
			subChunkIndicesToMarkDirty[chunkPos + glm::ivec2(1, 0)].push_back(subChunkIndex);
		}

		if (localPos.y % WorldConstants::CHUNK_SIZE == 0)
		{
			subChunkIndicesToMarkDirty[chunkPos].push_back(subChunkIndex - 1);
		}
		else if (localPos.y % WorldConstants::CHUNK_SIZE == WorldConstants::CHUNK_SIZE - 1)
		{
			subChunkIndicesToMarkDirty[chunkPos].push_back(subChunkIndex + 1);
		}

		if (localPos.z == 0)
		{
			subChunkIndicesToMarkDirty[chunkPos + glm::ivec2(0, -1)].push_back(subChunkIndex);
		}
		else if (localPos.z == WorldConstants::CHUNK_SIZE - 1)
		{
			subChunkIndicesToMarkDirty[chunkPos + glm::ivec2(0, 1)].push_back(subChunkIndex);
		}

		std::shared_lock lock(m_MutexChunkMeshes);
		for (const auto& [neighborChunkPos, neighborSubChunkIndex] : subChunkIndicesToMarkDirty)
		{
			auto it = m_ChunkMeshes.find(neighborChunkPos);
			if (it != m_ChunkMeshes.end())
			{
				for (uint8_t subChunkIndex : neighborSubChunkIndex)
				{
					it->second->SetSubChunkMeshDirty(subChunkIndex, true);
				}
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
				m_MeshBuilder.UpdateChunkMeshAsync(chunkMesh);
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

		// ----- General Stats -----
		ImGui::Text("Chunk Meshes: %s", FormatThousands(GetChunkMeshesCount()).c_str());
		ImGui::Text("Vertices: %s", FormatThousands(GetVertexCount()).c_str());
		const double avgMeshBuildTime_ms = m_MeshBuilder.GetAverageChunkMeshUpdateTime();
		ImGui::Text("Average Mesh Build Time: %.2f ms", avgMeshBuildTime_ms);
		ImGui::Text("Chunk Mesh Updates per Second: %.2f", m_MeshBuilder.GetChunkMeshUpdatesPerSecond());

		// ----- Render Distance -----
		ImGui::Separator();
		int chunkPersistanceDistance = m_WorldManager->GetChunkPersistanceDistance();
		if (ImGui::SliderInt("Chunk Persistence Distance", (int*) &chunkPersistanceDistance, 1, 20))
		{
			m_WorldManager->SetChunkPersistanceDistance(static_cast<uint8_t>(chunkPersistanceDistance));
		}

		// ----- Clear all chunks button -----
		ImGui::Separator();
		if (ImGui::Button("Clear All Chunks"))
		{
			m_WorldManager->RemoveAllChunks();
		}

		// ----- Render Options -----
		ImGui::Separator();
		ImGui::Checkbox("Render Chunk Borders", &m_RenderChunkBorders);

		// ----- Performance Options -----
		ImGui::Separator();
		size_t meshBuilderThreadCount = m_MeshBuilder.GetMeshBuilderThreadCount();
		if (ImGui::SliderInt(
				"Mesh Builder Thread Count", (int*) &meshBuilderThreadCount, 1, std::thread::hardware_concurrency()))
		{
			m_MeshBuilder.SetMeshBuilderThreadCount(meshBuilderThreadCount);
		}

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

	void WorldRenderer::Unload()
	{
		DeleteChunkMeshesAsync();
		DeleteChunkMeshes();

		m_Skybox.Unload();
		m_TextureAtlas->Unload();
	}

	void WorldRenderer::StaticUnload()
	{
		SubChunkMesh::s_Shader.Delete();
		SubChunkMesh::s_TextureAtlas.Delete();
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
