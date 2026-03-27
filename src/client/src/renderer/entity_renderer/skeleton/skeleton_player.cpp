#include "skeleton_player.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <numbers>

namespace onion::voxel
{

	SkeletonPlayer::SkeletonPlayer(const glm::vec3& position, const glm::vec3& facingDirection, float scale)
		: Skeleton(position, facingDirection, scale)
	{
		BuildSkeleton();
	}

	void SkeletonPlayer::SetPosition(const glm::vec3& position)
	{
		std::cerr << "[SkeletonPlayer::SetPosition] Not implemented yet!" << std::endl;
		assert(false && "Not implemented yet!");
	}

	void SkeletonPlayer::SetFacingDirection(const glm::vec3& facingDirection)
	{
		std::cerr << "[SkeletonPlayer::SetFacingDirection] Not implemented yet!" << std::endl;
		assert(false && "Not implemented yet!");
	}

	void SkeletonPlayer::SetScale(float scale)
	{
		std::cerr << "[SkeletonPlayer::SetScale] Not implemented yet!" << std::endl;
		assert(false && "Not implemented yet!");
	}

	static inline float SinusEmphasis(float input, float intensity)
	{
		input = std::clamp(input, 0.0f, 1.0f);
		return intensity * std::cos(2.0f * std::numbers::pi_v<float> * input);
	}

	void SkeletonPlayer::SetState(const Entity::State state, float progress, float intensity)
	{

		const float maxArmSwing = 50.f;
		const float maxLegSwing = 40.f;
		const float WalkingRunningRatio = 0.7f; // Ratio of swing speed between walking and running

		const float idleArmSwing = 1.f;

		if (state == Entity::State::Walking)
		{
			const float ArmAngle = SinusEmphasis(progress, maxArmSwing * WalkingRunningRatio);
			const float LegAngle = SinusEmphasis(progress, maxLegSwing * WalkingRunningRatio);

			LeftArm.Rotate(m_RigPoint_LeftArm, glm::vec3{ArmAngle, 0.f, 0.f});
			RightArm.Rotate(m_RigPoint_RightArm, glm::vec3{-ArmAngle, 0.f, 0.f});

			LeftLeg.Rotate(m_RigPoint_LeftLeg, glm::vec3{LegAngle, 0.f, 0.f});
			RightLeg.Rotate(m_RigPoint_RightLeg, glm::vec3{-LegAngle, 0.f, 0.f});
		}

		if (state == Entity::State::Running)
		{
			const float ArmAngle = SinusEmphasis(progress, maxArmSwing);
			const float LegAngle = SinusEmphasis(progress, maxLegSwing);
			LeftArm.Rotate(m_RigPoint_LeftArm, glm::vec3{ArmAngle, 0.f, 0.f});
			RightArm.Rotate(m_RigPoint_RightArm, glm::vec3{-ArmAngle, 0.f, 0.f});
			LeftLeg.Rotate(m_RigPoint_LeftLeg, glm::vec3{LegAngle, 0.f, 0.f});
			RightLeg.Rotate(m_RigPoint_RightLeg, glm::vec3{-LegAngle, 0.f, 0.f});
		}

		if (state == Entity::State::Idle)
		{
			const float ArmAngle = SinusEmphasis(progress, idleArmSwing);
			LeftArm.Rotate(m_RigPoint_LeftArm, glm::vec3{ArmAngle, 0.f, 0.f});
			RightArm.Rotate(m_RigPoint_RightArm, glm::vec3{-ArmAngle, 0.f, 0.f});
		}
	}

	void SkeletonPlayer::RotateHead(const glm::vec3& facingDirection)
	{
		// Head.Rotate(m_RigPoint_Head, rotationAngles_deg_local);
	}

	void SkeletonPlayer::BuildSkeleton()
	{
		const glm::vec3 Up(0.f, 1.f, 0.f);
		const glm::vec3 PlayerBodyFacing = m_FacingDirection - glm::dot(m_FacingDirection, Up) * Up;

		glm::vec3 NormalizedLateralBody = glm::cross(PlayerBodyFacing, Up); // right-handed "right"
		if (glm::length2(NormalizedLateralBody) < 1e-8f)
		{ // Facing ~ parallel to Up -> pick a fallback axis
			NormalizedLateralBody = glm::cross(PlayerBodyFacing, glm::vec3(0, 0, 1));
			if (glm::length2(NormalizedLateralBody) < 1e-8f)
				NormalizedLateralBody = glm::cross(PlayerBodyFacing, glm::vec3(1, 0, 0));
		}
		NormalizedLateralBody = glm::normalize(NormalizedLateralBody);

		// Set the members parts sizes
		const glm::vec3 PlayerHeadSize = glm::vec3(8.f * m_Scale); // Width, Height, Depth
		const glm::vec3 PlayerBodySize =
			glm::vec3(8.f * m_Scale, 12.f * m_Scale, 4.f * m_Scale); // Width, Height, Depth
		const glm::vec3 PlayerLeftArmSize =
			glm::vec3(4.f * m_Scale, 12.f * m_Scale, 4.f * m_Scale); // Width, Height, Depth
		const glm::vec3 PlayerRightArmSize =
			glm::vec3(4.f * m_Scale, 12.f * m_Scale, 4.f * m_Scale); // Width, Height, Depth
		const glm::vec3 PlayerLeftLegSize =
			glm::vec3(4.f * m_Scale, 12.f * m_Scale, 4.f * m_Scale); // Width, Height, Depth
		const glm::vec3 PlayerRightLegSize =
			glm::vec3(4.f * m_Scale, 12.f * m_Scale, 4.f * m_Scale); // Width, Height, Depth

		// Create the player Head (Reference position for other parts)
		float PlayerHeadHeightOffset = (PlayerHeadSize.y / 2) + (PlayerBodySize.y) + (PlayerRightLegSize.y);
		glm::vec3 PlayerHeadPos = m_Position + glm::vec3(0.0f, PlayerHeadHeightOffset, 0.0f);
		Head = Cuboid(PlayerHeadPos, PlayerHeadSize.x, PlayerHeadSize.y, PlayerHeadSize.z, m_FacingDirection, 0);

		// Add the Body Cuboid
		glm::vec3 PlayerBodyPos = PlayerHeadPos;
		PlayerBodyPos.y -= (PlayerHeadSize.y / 2.0f) + (PlayerBodySize.y / 2.0f); // Position body below head
		Body = Cuboid(PlayerBodyPos, PlayerBodySize.x, PlayerBodySize.y, PlayerBodySize.z, PlayerBodyFacing, 0);

		// Left Arm
		glm::vec3 PlayerLeftArmPos = PlayerBodyPos;
		PlayerLeftArmPos -= NormalizedLateralBody * ((PlayerBodySize.x / 2) + (PlayerLeftArmSize.x / 2));
		LeftArm = Cuboid(
			PlayerLeftArmPos, PlayerLeftArmSize.x, PlayerLeftArmSize.y, PlayerLeftArmSize.z, PlayerBodyFacing, 0);

		// Right Arm
		glm::vec3 PlayerRightArmPos = PlayerBodyPos;
		PlayerRightArmPos += NormalizedLateralBody * ((PlayerBodySize.x / 2) + (PlayerRightArmSize.x / 2));
		RightArm = Cuboid(
			PlayerRightArmPos, PlayerRightArmSize.x, PlayerRightArmSize.y, PlayerRightArmSize.z, PlayerBodyFacing, 0);

		// Left Leg
		glm::vec3 PlayerLeftLegPos = PlayerBodyPos;
		PlayerLeftLegPos.y -= (PlayerBodySize.y / 2.0f) + (PlayerLeftLegSize.y / 2.0f); // Position left leg below body
		PlayerLeftLegPos -= NormalizedLateralBody * (PlayerLeftLegSize.x / 2);
		LeftLeg = Cuboid(
			PlayerLeftLegPos, PlayerLeftLegSize.x, PlayerLeftLegSize.y, PlayerLeftLegSize.z, PlayerBodyFacing, 0);

		// Right Leg
		glm::vec3 PlayerRightLegPos = PlayerBodyPos;
		PlayerRightLegPos.y -=
			(PlayerBodySize.y / 2.0f) + (PlayerRightLegSize.y / 2.0f); // Position right leg below body
		PlayerRightLegPos += NormalizedLateralBody * (PlayerRightLegSize.x / 2);
		RightLeg = Cuboid(
			PlayerRightLegPos, PlayerRightLegSize.x, PlayerRightLegSize.y, PlayerRightLegSize.z, PlayerBodyFacing, 0);

		// Set the rig points (rotation points) for each part in world space
		m_RigPoint_Head =
			m_Position + glm::vec3{0.f, (PlayerBodySize.y) + (PlayerRightLegSize.y), 0.f}; // Neck position
		m_RigPoint_Body =
			m_Position + glm::vec3{0.f, (PlayerBodySize.y / 2.f) + (PlayerRightLegSize.y), 0.f}; // Center of body
		m_RigPoint_LeftArm = PlayerLeftArmPos +
			glm::vec3{0.f, (PlayerLeftArmSize.y / 2.f) - (PlayerLeftArmSize.x / 2), 0.f}; // Top of left arm
		m_RigPoint_RightArm = PlayerRightArmPos +
			glm::vec3{0.f, (PlayerRightArmSize.y / 2.f) - (PlayerRightArmSize.x / 2), 0.f};		  // Top of right arm
		m_RigPoint_LeftLeg = PlayerLeftLegPos + glm::vec3{0.f, (PlayerLeftLegSize.y / 2.f), 0.f}; // Top of left leg
		m_RigPoint_RightLeg = PlayerRightLegPos +
			glm::vec3{0.f, (PlayerRightLegSize.y / 2.f) + (PlayerRightLegSize.x / 2), 0.f}; // Top of right leg
	}
} // namespace onion::voxel
