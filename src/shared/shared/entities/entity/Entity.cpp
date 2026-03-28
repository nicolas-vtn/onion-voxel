#include "Entity.hpp"

#include <cassert>

namespace onion::voxel
{
	Entity::Entity(EntityType type, const std::string& uuid) : Type(type), UUID(uuid) {}

	Entity::~Entity() {}

	bool Entity::HasPhysicsBody() const
	{
		std::shared_lock lock(m_MutexPhysicsBody);
		return m_PhysicsBody.has_value();
	}

	PhysicsBody Entity::GetPhysicsBody() const
	{
		assert(m_PhysicsBody.has_value() && "Entity must have a PhysicsBody component to get it.");
		std::shared_lock lock(m_MutexPhysicsBody);
		return *m_PhysicsBody;
	}

	void Entity::SetPhysicsBody(const PhysicsBody& physicsBody)
	{
		std::unique_lock lock(m_MutexPhysicsBody);
		m_PhysicsBody = physicsBody;
	}

	bool Entity::HasTransform() const
	{
		std::shared_lock lock(m_MutexTransform);
		return m_Transform.has_value();
	}

	Transform Entity::GetTransform() const
	{
		assert(m_Transform.has_value() && "Entity must have a Transform component to get it.");
		std::shared_lock lock(m_MutexTransform);
		return *m_Transform;
	}

	void Entity::SetTransform(const Transform& transform)
	{
		std::unique_lock lock(m_MutexTransform);
		m_Transform = transform;
	}

	glm::vec3 Entity::GetPosition() const
	{
		assert(m_Transform.has_value() && "Entity must have a Transform component to get its position.");
		std::shared_lock lock(m_MutexTransform);
		return m_Transform->Position;
	}

	void Entity::SetPosition(const glm::vec3& position)
	{
		assert(m_Transform.has_value() && "Entity must have a Transform component to set its position.");
		std::unique_lock lock(m_MutexTransform);
		m_Transform->Position = position;
	}

	glm::vec3 Entity::GetFacing() const
	{
		assert(m_Transform.has_value() && "Entity must have a Transform component to get its facing direction.");

		std::shared_lock lock(m_MutexTransform);

		// Calculate facing direction from rotation (assuming Y-axis is up)
		float yaw = m_Transform->Rotation.y;
		float pitch = m_Transform->Rotation.x;
		glm::vec3 facing;
		facing.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		facing.y = sin(glm::radians(pitch));
		facing.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		return glm::normalize(facing);
	}

	void Entity::SetFacing(const glm::vec3& facing)
	{
		assert(m_Transform.has_value() && "Entity must have a Transform component to set its facing direction.");

		std::unique_lock lock(m_MutexTransform);

		// Calculate yaw and pitch from facing direction
		float yaw = glm::degrees(atan2(facing.z, facing.x));
		float pitch = glm::degrees(asin(facing.y));
		m_Transform->Rotation.y = yaw;
		m_Transform->Rotation.x = pitch;
	}

	Entity::State Entity::GetState() const
	{
		std::shared_lock lock(m_Mutex);
		return m_State;
	}

	void Entity::SetState(const State state)
	{
		std::unique_lock lock(m_Mutex);
		m_State = state;
	}

} // namespace onion::voxel
