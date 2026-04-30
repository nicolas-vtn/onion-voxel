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

			if (lastBlock.ID == BlockId::Air)
			{
				previousPos = currentPos;
				continue;
			}

			// Non-air cell — run slab ray-AABB test against each element.
			// The cell may contain empty space (non-full blocks), so a geometry
			// hit is not guaranteed just because the cell is non-air.
			glm::ivec3 faceNormal{0};
			glm::vec3 hitPosition = currentPos;
			bool geometryHit = false;

			const auto& blockstates = BlockstateRegistry::Get();
			auto it = blockstates.find(lastBlock.ID);
			if (it != blockstates.end())
			{
				const auto& elements = it->second[lastBlock.VariantIndex].Model.Elements;
				float bestT = std::numeric_limits<float>::max();

				glm::vec3 invDir = 1.f / dirNormalized;

				for (const auto& element : elements)
				{
					glm::vec3 aabbMin = glm::vec3(cellPos) + element.From / 16.f;
					glm::vec3 aabbMax = glm::vec3(cellPos) + element.To / 16.f;

					glm::vec3 t1 = glm::min((aabbMin - origin) * invDir, (aabbMax - origin) * invDir);
					glm::vec3 t2 = glm::max((aabbMin - origin) * invDir, (aabbMax - origin) * invDir);

					float tEntry = glm::max(glm::max(t1.x, t1.y), t1.z);
					float tExit = glm::min(glm::min(t2.x, t2.y), t2.z);

					// Small epsilon handles the case where the ray origin sits exactly
					// on the element surface due to floating-point precision.
					if (tEntry <= tExit && tEntry >= -1e-4f && tEntry < bestT)
					{
						bestT = tEntry;
						geometryHit = true;

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
			else
			{
				// Registry gap — unknown block, treat cell as a full cube hit
				geometryHit = true;
				glm::vec3 delta = glm::vec3(cellPos) - glm::floor(previousPos);
				faceNormal = glm::ivec3(-delta);
				hitPosition = currentPos;
			}

			if (!geometryHit)
			{
				// Ray is passing through the empty space of a non-full block cell — keep marching
				previousPos = currentPos;
				continue;
			}

			RaycastHit hit;

			hit.HitPosition = hitPosition;
			hit.HitFaceNormal = faceNormal;

			Block& hitBlock = hit.HitBlock;
			hitBlock.Position = cellPos;
			hitBlock.State = lastBlock;

			Block& adjacent = hit.AdjacentBlock;
			adjacent.Position = cellPos + faceNormal;
			adjacent.State = worldManager.GetBlock(adjacent.Position);

			return hit;
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
