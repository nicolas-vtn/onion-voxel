#pragma once

#include "skeleton.hpp"

#include <shared/entities/entity/player/Player.hpp>

namespace onion::voxel
{

	class SkeletonPlayer : public Skeleton
	{

		// ------- CONSTRUCTORS & DESTRUCTOR -------
	  public:
		SkeletonPlayer(const glm::vec3& position, const glm::vec3& facingDirection, float scale);
		~SkeletonPlayer() override = default;

		// ------- SETTERS -------
		void SetPosition(const glm::vec3& position) override;
		void SetFacingDirection(const glm::vec3& facingDirection) override;
		void SetScale(float scale) override;
		void SetState(const Entity::State state, float progress, float intensity = 1.f) override;

	  public:
		void RotateHead(const glm::vec3& facingDirection);

	  public:
		Cuboid Head;
		Cuboid Body;
		Cuboid LeftArm;
		Cuboid RightArm;
		Cuboid LeftLeg;
		Cuboid RightLeg;

	  private:
		glm::vec3 m_RigPoint_Head{0.f, 0.f, 0.f};
		glm::vec3 m_RigPoint_Body{0.f, 0.f, 0.f};
		glm::vec3 m_RigPoint_LeftArm{0.f, 0.f, 0.f};
		glm::vec3 m_RigPoint_RightArm{0.f, 0.f, 0.f};
		glm::vec3 m_RigPoint_LeftLeg{0.f, 0.f, 0.f};
		glm::vec3 m_RigPoint_RightLeg{0.f, 0.f, 0.f};

	  private:
		void BuildSkeleton();
	};

} // namespace onion::voxel
