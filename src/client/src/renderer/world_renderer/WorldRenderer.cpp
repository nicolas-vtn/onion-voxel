#include "WorldRenderer.hpp"

namespace onion::voxel
{
	WorldRenderer::WorldRenderer(std::shared_ptr<Camera> camera) : m_Camera(camera) {};
	WorldRenderer::~WorldRenderer() {};

	void WorldRenderer::Render()
	{
		// Get Camera projection, view and ProjView Matix
		glm::mat4 projectionMatrix = m_Camera->GetProjectionMatrix();
		glm::mat4 viewMatrix = m_Camera->GetViewMatrix();
		glm::mat4 viewProjMatrix = projectionMatrix * viewMatrix;
	}

} // namespace onion::voxel
