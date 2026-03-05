#pragma once

#include "../camera/camera.hpp"

#include <memory>

namespace onion::voxel
{
	class WorldRenderer
	{
		// ----- Constructor / Destructor -----
	  public:
		WorldRenderer(std::shared_ptr<Camera> camera);
		~WorldRenderer();

		// ----- Public API -----
	  public:
		void Render();

		// ----- Getters / Setters -----
	  public:
		// ----- Camera -----
	  private:
		std::shared_ptr<Camera> m_Camera;
	};
} // namespace onion::voxel
