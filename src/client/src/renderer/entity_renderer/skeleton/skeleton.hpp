#pragma once

#include <renderer/entity_renderer/cuboid.hpp>

namespace onion::voxel
{

	class Skeleton
	{
		// ------- FORWARD DECLARATIONS -------
	  public:
		enum class State : uint8_t
		{
			IDLE = 0,
			WALKING = 1,
			RUNNING = 2,
			JUMPING,
			FALLING,
			ATTACKING,
			DYING
		};

	  public:
		// ------- CONSTRUCTORS & DESTRUCTOR -------
		Skeleton() = default;
		virtual ~Skeleton() = default;

		// ------- GETTERS -------
	  public:
		glm::vec3 GetPosition() const;
		glm::vec3 GetFacingDirection() const;
		float GetScale() const;

		// ------- SETTERS -------
	  public:
		virtual void SetPosition(const glm::vec3& position) = 0;
		virtual void SetFacingDirection(const glm::vec3& facingDirection) = 0;
		virtual void SetScale(float scale) = 0;

		/// <summary>
		/// Defines the state of the skeleton.
		/// </summary>
		/// <param name="state">State to set.</param>
		/// <param name="progress">Progression in range [0,1].</param>
		/// <param name="intensity">Intensity in range [0,1].</param>
		virtual void SetState(const State state, float progress, float intensity = 1.f) = 0;

		// ------- PROTECTED VARIABLES -------
	  protected:
		glm::vec3 m_Position{0, 0, 0};
		glm::vec3 m_FacingDirection{0, 0, 1};
		float m_Scale = 1.0f;

		Skeleton(const glm::vec3& position, const glm::vec3& facingDirection, float scale);
	};

} // namespace onion::voxel
