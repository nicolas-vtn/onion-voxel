#include "BlockEntityMesh.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <renderer/world_renderer/chunk_mesh/MeshBuilder.hpp>

namespace onion::voxel
{
	void BlockEntityMesh::Render(const glm::mat4& modelMatrix, const glm::mat4& viewProjMatrix)
	{
		s_Shader.Use();
		s_Shader.setMat4("u_ModelMatrix", modelMatrix);
		s_Shader.setVec2("u_PositionOffset", glm::vec2(0.0f, 0.0f));
		s_Shader.setMat4("u_ViewProjMatrix", viewProjMatrix);
		s_Shader.setVec3("u_LightColor", GetLightColor());
		s_Shader.setBool("u_UseFaceShading", GetUseFaceShading());

		if (m_TextureAtlas)
			m_TextureAtlas->Bind();

		PrepareForRendering();

		// Opaque pass
		PrepareForRenderingOpaque();
		RenderOpaque();

		// Cutout pass — two sub-passes (back faces first, then front faces with offset)
		PrepareForRenderingCutout();

		glCullFace(GL_FRONT);
		glDisable(GL_POLYGON_OFFSET_FILL);
		RenderCutout();

		glCullFace(GL_BACK);
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(-1.0f, -1.0f);
		RenderCutout();

		// Transparent pass
		PrepareForRenderingTransparent();
		RenderTransparent();

		ResetOpenGLState();
	}

	std::shared_ptr<BlockEntityMesh> BlockEntityMesh::GetOrCreate(BlockId blockId, const MeshBuilder& meshBuilder)
	{
		std::lock_guard lock(s_CacheMutex);

		auto it = s_Cache.find(blockId);
		if (it != s_Cache.end())
			return it->second;

		auto mesh = std::make_shared<BlockEntityMesh>();
		meshBuilder.BuildBlockEntityMesh(*mesh, blockId);
		s_Cache[blockId] = mesh;
		return mesh;
	}

	void BlockEntityMesh::ClearCache()
	{
		std::lock_guard lock(s_CacheMutex);

		for (auto& [id, mesh] : s_Cache)
			mesh->Delete();

		s_Cache.clear();
	}

} // namespace onion::voxel
