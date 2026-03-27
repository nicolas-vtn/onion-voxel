#include "skeleton.hpp"

namespace onion::voxel
{

	Skeleton::Skeleton(const glm::vec3& position, const glm::vec3& facingDirection, float scale)
		: m_Position(position), m_FacingDirection(facingDirection), m_Scale(scale)
	{
	}

	glm::vec3 Skeleton::GetPosition() const
	{
		return m_Position;
	}

	glm::vec3 Skeleton::GetFacingDirection() const
	{
		return m_FacingDirection;
	}

	float Skeleton::GetScale() const
	{
		return m_Scale;
	}
} // namespace onion::voxel
