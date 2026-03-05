#pragma once

#include <memory>

#include <shared/world/world_manager/WorldManager.hpp>

#include "../camera/camera.hpp"

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
		void Render();

		// ----- Getters / Setters -----
	  public:
		// ----- World Manager -----
	  private:
		std::shared_ptr<WorldManager> m_WorldManager;

		// ----- Camera -----
	  private:
		std::shared_ptr<Camera> m_Camera;
	};
} // namespace onion::voxel
