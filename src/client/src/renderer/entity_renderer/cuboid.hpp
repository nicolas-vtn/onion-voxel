#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace onion::voxel
{

	/// <summary>
	/// 3D Cuboid class with position, size, orientation and methods to get the positions of its faces.
	/// Local space is defined as:
	/// - Centered at the cuboid's center.
	/// - Width along the X axis.
	/// - Height along the Y axis.
	/// - Depth along the Z axis.
	/// </summary>
	class Cuboid
	{
	  public:
		// ------- STRUCTS -------
		struct FacePositions
		{
			glm::vec3 BottomLeft;
			glm::vec3 BottomRight;
			glm::vec3 TopRight;
			glm::vec3 TopLeft;
		};

		// ------- CONSTRUCTORS & DESTRUCTOR -------
		Cuboid() = default;

		Cuboid(const glm::vec3& center,
			   float width,
			   float height,
			   float depth,
			   const glm::vec3& facingDirection,
			   float rotation_deg);

		~Cuboid() = default;

		// ------- GETTERS -------
	  public:
		FacePositions GetFrontFacePositions() const;
		FacePositions GetBackFacePositions() const;
		FacePositions GetLeftFacePositions() const;
		FacePositions GetRightFacePositions() const;
		FacePositions GetTopFacePositions() const;
		FacePositions GetBottomFacePositions() const;

		glm::vec3 GetCenter() const { return m_Center; }

		// ------- MEMBER VARIABLES -------
	  private:
		glm::vec3 m_Center{0, 0, 0};
		float m_Width = 1.0f;
		float m_Height = 1.0f;
		float m_Depth = 1.0f;

		glm::vec3 m_FacingDirection = glm::vec3(0, 0, 1); // Default facing direction

		glm::quat m_Orientation = glm::quat(1, 0, 0, 0); // identity
		float m_Roration_degrees = 0.0f;				 // Rotation around the FacingDirection axis in degrees

		// ------- PUBLIC METHODS -------
	  public:
		void Rotate(const glm::vec3& rotationPoint, const glm::vec3& rotationAngles_deg_local);

		// ------- UPDATE POSITIONS -------
	  private:
		void UpdateFinalPositions();

		// ------- FINAL POSITIONS AFTER ROTATION & TRANSLATION -------
	  private:
		glm::vec3 m_FinalPosFrontBottomLeft{0, 0, 0};
		glm::vec3 m_FinalPosFrontBottomRight{1, 0, 0};
		glm::vec3 m_FinalPosFrontTopLeft{0, 1, 0};
		glm::vec3 m_FinalPosFrontTopRight{1, 1, 0};
		glm::vec3 m_FinalPosBackBottomLeft{0, 0, 1};
		glm::vec3 m_FinalPosBackBottomRight{1, 0, 1};
		glm::vec3 m_FinalPosBackTopLeft{0, 1, 1};
		glm::vec3 m_FinalPosBackTopRight{1, 1, 1};
	};
} // namespace onion::voxel
