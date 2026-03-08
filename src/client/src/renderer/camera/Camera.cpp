#include "Camera.hpp"

namespace onion::voxel
{
	Camera::Camera(glm::vec3 startPosition, int screenWidth, int screenHeight)
		: m_Position(startPosition), m_Front(glm::vec3(0.0f, 0.0f, -1.0f)), m_Up(glm::vec3(0.0f, 1.0f, 0.0f)),
		  m_Yaw(-90.0f), m_Pitch(0.0f)
	{
		SetAspectRatio(static_cast<float>(screenWidth) / static_cast<float>(screenHeight));
		UpdateProjectionMatrix();
	}

	Camera::~Camera()
	{
		// Destructor implementation
	}

	glm::mat4 Camera::GetViewMatrix() const
	{
		return glm::lookAt(m_Position, m_Position + m_Front, m_Up);
	}

	void Camera::UpdateYawPitchFromFront()
	{
		glm::vec3 normalizedFront = glm::normalize(m_Front);

		m_Pitch = glm::degrees(asin(normalizedFront.y));
		m_Yaw = glm::degrees(atan2(normalizedFront.z, normalizedFront.x));
	}

	void Camera::UpdateFrontFromYawPitch()
	{
		glm::vec3 front;
		front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
		front.y = sin(glm::radians(m_Pitch));
		front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
		m_Front = glm::normalize(front);
	}

	glm::vec3 Camera::GetPosition() const
	{
		return m_Position;
	}

	void Camera::SetPosition(const glm::vec3& position)
	{
		m_Position = position;
	}

	glm::vec3 Camera::GetFront() const
	{
		return m_Front;
	}

	void Camera::SetFront(const glm::vec3& front)
	{
		m_Front = front;
		UpdateYawPitchFromFront();
	}

	glm::vec3 Camera::GetUp() const
	{
		return m_Up;
	}

	float Camera::GetYaw() const
	{
		return m_Yaw;
	}

	void Camera::SetYaw(float yaw)
	{
		m_Yaw = yaw;
		UpdateFrontFromYawPitch();
	}

	float Camera::GetPitch() const
	{
		return m_Pitch;
	}

	void Camera::SetPitch(float pitch)
	{
		m_Pitch = pitch;
		UpdateFrontFromYawPitch();
	}

	glm::mat4 Camera::GetProjectionMatrix() const
	{
		return m_Projection;
	}

	glm::mat4 Camera::GetViewProjectionMatrix() const
	{
		return GetProjectionMatrix() * GetViewMatrix();
	}

	void Camera::UpdateProjectionMatrix()
	{
		m_Projection = glm::perspective(glm::radians(m_FovY), m_AspectRatio, m_NearPlane, m_FarPlane);
	}

	void Camera::SetAspectRatio(float aspectRatio)
	{
		m_AspectRatio = aspectRatio;
		m_UpdatedProjectionMatrix = true;
		UpdateProjectionMatrix();
	}

	void Camera::SetFovY(float fovY)
	{
		m_FovY = fovY;
		m_UpdatedProjectionMatrix = true;
		UpdateProjectionMatrix();
	}

	float Camera::GetAspectRatio() const
	{
		return m_AspectRatio;
	}

	bool Camera::IsProjectionMatrixUpdated() const
	{
		return m_UpdatedProjectionMatrix;
	}

	void Camera::ResetProjectionMatrixUpdateFlag()
	{
		m_UpdatedProjectionMatrix = false;
	}

	float Camera::GetFovY() const
	{
		return m_FovY;
	}

} // namespace onion::voxel
