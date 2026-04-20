#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace onion::voxel
{
	class Camera
	{
		// ----- Constructor / Destructor -----
	  public:
		Camera(glm::vec3 startPosition, int screenWidth, int screenHeight);
		~Camera();

		/// ----- Public API -----
	  public:
		glm::mat4 GetViewMatrix() const;

		// ----- Positions -----
	  private:
		glm::vec3 m_Position; // Camera position in world space
		glm::vec3 m_Front;	  // Direction the camera is looking at (normalized)
		glm::vec3 m_Up;		  // Up direction of the camera (normalized)
		float m_Yaw;		  // Yaw angle in degrees
		float m_Pitch;		  // Pitch angle in degrees

		// ----- Projection Parameters -----
	  private:
		float m_Fov = 75.0f;				// Field of view in degrees
		float m_AspectRatio = 16.0f / 9.0f; // Aspect ratio
		float m_NearPlane = 0.1f;			// Near clipping plane
		float m_FarPlane = 2000.f;			// Far clipping plane

		// ----- Internal Methods -----
	  private:
		void UpdateFrontFromYawPitch();
		void UpdateYawPitchFromFront();

		glm::mat4 m_Projection = glm::perspective(glm::radians(m_Fov), m_AspectRatio, m_NearPlane, m_FarPlane);
		bool m_UpdatedProjectionMatrix = false;

		// ----- Getters / Setters -----
	  public:
		glm::vec3 GetPosition() const;
		void SetPosition(const glm::vec3& position);

		glm::vec3 GetFront() const;
		void SetFront(const glm::vec3& front);

		glm::vec3 GetUp() const;

		float GetYaw() const;
		void SetYaw(float yaw);

		float GetPitch() const;
		void SetPitch(float pitch);

		float GetFov() const;
		void SetFov(float fov);

		glm::mat4 GetProjectionMatrix() const;
		glm::mat4 GetViewProjectionMatrix() const;
		glm::mat4 GetUntranslatedViewProjectionMatrix() const;

		void UpdateProjectionMatrix();

		void SetAspectRatio(float aspectRatio);

		float GetAspectRatio() const;

		bool IsProjectionMatrixUpdated() const;

		void ResetProjectionMatrixUpdateFlag();
	};
} // namespace onion::voxel
