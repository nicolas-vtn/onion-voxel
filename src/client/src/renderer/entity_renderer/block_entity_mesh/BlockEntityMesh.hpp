#pragma once

#include <glm/glm.hpp>

#include <memory>
#include <mutex>
#include <unordered_map>

#include <renderer/item_mesh/ItemMesh.hpp>

#include <shared/world/block/BlockId.hpp>

namespace onion::voxel
{
	class MeshBuilder;

	/// @brief A world-space block mesh used to render dropped BlockEntity items.
	/// Inherits the three-pass VAO/VBO infrastructure from ItemMesh.
	/// Meshes are built once per BlockId and cached statically.
	class BlockEntityMesh : public ItemMesh
	{
		friend class MeshBuilder;

		// ----- Constructor / Destructor -----
	  public:
		BlockEntityMesh() = default;
		~BlockEntityMesh() = default;

		// ----- Public API -----
	  public:
		/// @brief Render this mesh with a world-space model matrix and a camera view-projection matrix.
		/// @param modelMatrix  Encodes world translation + Y-rotation + bob offset.
		/// @param viewProjMatrix  Camera VP matrix (perspective).
		void Render(const glm::mat4& modelMatrix, const glm::mat4& viewProjMatrix);

		// ----- Static Cache -----
	  public:
		/// @brief Returns a cached BlockEntityMesh for @p blockId, building it if necessary.
		static std::shared_ptr<BlockEntityMesh> GetOrCreate(BlockId blockId, const MeshBuilder& meshBuilder);

		/// @brief Deletes and clears the entire cache. Call from the GL thread when textures are reloaded.
		static void ClearCache();

	  private:
		static inline std::mutex s_CacheMutex;
		static inline std::unordered_map<BlockId, std::shared_ptr<BlockEntityMesh>> s_Cache;
	};

} // namespace onion::voxel
