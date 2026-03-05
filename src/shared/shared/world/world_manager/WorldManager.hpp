#pragma once

#include <filesystem>
#include <memory>

#include <onion/Event.hpp>

#include "../chunk/Chunk.hpp"

namespace onion::voxel
{
	class WorldManager
	{
		// ----- Constructor / Destructor -----
	  public:
		WorldManager();
		~WorldManager();

		// ----- Public API -----
	  public:
		void LoadWorld(const std::filesystem::path& worldPath);
		void UnloadWorld();

		void AddChunk(const std::shared_ptr<void>& chunkData);
		void RemoveChunk(const std::shared_ptr<void>& chunkData);

		// ----- Events -----
	  public:
		Event<std::shared_ptr<void>> ChunkAdded;
		Event<std::shared_ptr<void>> ChunkRemoved;

	  private:
		std::filesystem::path m_CurrentWorldPath;
	};
} // namespace onion::voxel
