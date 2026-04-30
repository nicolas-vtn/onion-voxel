#include "Raycast.hpp"

#include <limits>

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
		glm::ivec3 lastCellPos = glm::ivec3(std::numeric_limits<int>::min());
		BlockState lastBlock{};

		for (int i = 0; i <= steps; i++)
		{
			glm::vec3 currentPos = origin + step * static_cast<float>(i);
			glm::ivec3 cellPos = glm::floor(currentPos);

			// Only re-fetch when the cell changes
			if (cellPos != lastCellPos)
			{
				lastCellPos = cellPos;
				lastBlock = worldManager.GetBlock(cellPos);
			}

			BlockState block = lastBlock;

			if (block.ID != BlockId::Air)
			{
				RaycastHit hit;

				Block& hitBlock = hit.HitBlock;
				hitBlock.Position = cellPos;
				hitBlock.State = block;

				glm::ivec3 faceNormal{0};
				glm::vec3 hitPosition = currentPos; // fallback: first sample inside cell

				// Single registry fetch + slab ray-AABB test over all elements.
				// Replaces both the point-in-AABB test in GetBlock(vec3) and the
				// cell-delta face heuristic, giving correct results for non-full blocks.
				const auto& blockstates = BlockstateRegistry::Get();
				auto it = blockstates.find(block.ID);
				if (it != blockstates.end())
				{
					const auto& elements = it->second[block.VariantIndex].Model.Elements;
					float bestT = std::numeric_limits<float>::max();

					for (const auto& element : elements)
					{
						glm::vec3 aabbMin = glm::vec3(cellPos) + element.From / 16.f;
						glm::vec3 aabbMax = glm::vec3(cellPos) + element.To / 16.f;

						glm::vec3 invDir = 1.f / dirNormalized;
						glm::vec3 t1 = glm::min((aabbMin - origin) * invDir, (aabbMax - origin) * invDir);
						glm::vec3 t2 = glm::max((aabbMin - origin) * invDir, (aabbMax - origin) * invDir);

						float tEntry = glm::max(glm::max(t1.x, t1.y), t1.z);
						float tExit = glm::min(glm::min(t2.x, t2.y), t2.z);

						if (tEntry <= tExit && tEntry >= 0.f && tEntry < bestT)
						{
							bestT = tEntry;

							// Axis with the largest t1 is the entry face
							int axis = 0;
							if (t1.y >= t1.x && t1.y >= t1.z)
								axis = 1;
							else if (t1.z >= t1.x)
								axis = 2;

							glm::ivec3 n{0};
							n[axis] = (dirNormalized[axis] < 0.f) ? 1 : -1;
							faceNormal = n;
							hitPosition = origin + dirNormalized * bestT;
						}
					}
				}

				// Fallback: registry gap or zero elements — use cell-delta heuristic
				if (faceNormal == glm::ivec3{0})
				{
					glm::vec3 delta = glm::vec3(cellPos) - glm::floor(previousPos);
					faceNormal = glm::ivec3(-delta);
					hitPosition = currentPos;
				}

				hit.HitPosition = hitPosition;
				hit.HitFaceNormal = faceNormal;

				Block& adjacent = hit.AdjacentBlock;
				adjacent.Position = cellPos + faceNormal;
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
