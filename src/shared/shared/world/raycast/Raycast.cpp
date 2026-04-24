#include "Raycast.hpp"

namespace onion::voxel
{
	std::optional<RaycastHit> Raycaster::Raycast(const WorldManager& worldManager,
												 const glm::vec3& origin,
												 const glm::vec3& direction,
												 float maxDistance,
												 int steps)
	{
		glm::vec3 dirNormalized = glm::normalize(direction);
		glm::vec3 step = dirNormalized * (maxDistance / static_cast<float>(steps));

		glm::vec3 previousPos = origin;

		for (int i = 0; i <= steps; i++)
		{
			glm::vec3 currentPos = origin + step * static_cast<float>(i);
			BlockState block = worldManager.GetBlock(currentPos);

			if (block.ID != BlockId::Air)
			{
				RaycastHit hit;
				hit.HitPosition = currentPos;

				glm::vec3 hitBlockPos = glm::floor(currentPos);
				glm::vec3 prevBlockPos = glm::floor(previousPos);

				Block& hitBlock = hit.HitBlock;
				hitBlock.Position = hitBlockPos;
				hitBlock.State = block;

				// Compute entry direction (which face was hit)
				glm::vec3 delta = hitBlockPos - prevBlockPos;

				// Adjacent block is the block we came from
				Block& adjacent = hit.AdjacentBlock;
				adjacent.Position = hitBlockPos - delta;
				adjacent.State = worldManager.GetBlock(adjacent.Position);

				return hit;
			}

			previousPos = currentPos;
		}

		return std::nullopt; // No hit
	}

	std::optional<RaycastHit>
	Raycaster::Raycast(const WorldManager& worldManager, const glm::vec3& origin, const glm::vec3& target, float steps)
	{
		glm::vec3 direction = target - origin;
		float distance = glm::length(direction);

		if (distance == 0.0f)
			return std::nullopt; // Origin and target are the same

		return Raycast(worldManager, origin, direction, distance, static_cast<int>(steps));
	}
} // namespace onion::voxel
