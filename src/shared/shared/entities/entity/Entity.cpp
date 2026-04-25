#include "Entity.hpp"

#include <cassert>
#include <mutex>

namespace onion::voxel
{
	Entity::Entity(EntityType type, const std::string& uuid) : Type(type), UUID(uuid) {}

	Entity::~Entity() {}

	PhysicsBody Entity::GetPhysicsBody() const
	{
		std::shared_lock lock(m_MutexPhysicsBody);
		assert(m_PhysicsBody.has_value() && "Entity must have a PhysicsBody component to get it.");
		return *m_PhysicsBody;
	}

	Transform Entity::GetTransform() const
	{
		std::shared_lock lock(m_MutexTransform);
		assert(m_Transform.has_value() && "Entity must have a Transform component to get it.");
		return *m_Transform;
	}

	glm::vec3 Entity::GetPosition() const
	{
		std::shared_lock lock(m_MutexTransform);
		assert(m_Transform.has_value() && "Entity must have a Transform component to get its position.");
		return m_Transform->Position;
	}

	glm::vec3 Entity::GetFacing() const
	{
		std::shared_lock lock(m_MutexTransform);
		assert(m_Transform.has_value() && "Entity must have a Transform component to get its facing direction.");

		float yaw = m_Transform->Rotation.y;
		float pitch = m_Transform->Rotation.x;

		glm::vec3 facing;
		facing.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		facing.y = sin(glm::radians(pitch));
		facing.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		return glm::normalize(facing);
	}

	bool Entity::HasPhysicsBody() const
	{
		std::shared_lock lock(m_MutexPhysicsBody);
		return m_PhysicsBody.has_value();
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

	void Entity::SetTransform(const Transform& transform)
	{
		std::unique_lock lock(m_MutexTransform);
		m_Transform = transform;
	}

	void Entity::SetPosition(const glm::vec3& position)
	{
		std::unique_lock lock(m_MutexTransform);
		assert(m_Transform.has_value() && "Entity must have a Transform component to set its position.");
		m_Transform->Position = position;
	}

	void Entity::SetFacing(const glm::vec3& facing)
	{
		std::unique_lock lock(m_MutexTransform);
		assert(m_Transform.has_value() && "Entity must have a Transform component to set its facing direction.");

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

	// ----- Health -----

	bool Entity::HasHealth() const
	{
		std::shared_lock lock(m_MutexHealth);
		return m_Health.has_value();
	}

	Health Entity::GetHealth() const
	{
		std::shared_lock lock(m_MutexHealth);
		assert(m_Health.has_value() && "Entity must have a Health component to get it.");
		return *m_Health;
	}

	void Entity::SetHealth(const Health& health)
	{
		std::unique_lock lock(m_MutexHealth);
		m_Health = health;
	}

	// ----- Hunger -----

	bool Entity::HasHunger() const
	{
		std::shared_lock lock(m_MutexHunger);
		return m_Hunger.has_value();
	}

	Hunger Entity::GetHunger() const
	{
		std::shared_lock lock(m_MutexHunger);
		assert(m_Hunger.has_value() && "Entity must have a Hunger component to get it.");
		return *m_Hunger;
	}

	void Entity::SetHunger(const Hunger& hunger)
	{
		std::unique_lock lock(m_MutexHunger);
		m_Hunger = hunger;
	}

	// ----- Experience -----

	bool Entity::HasExperience() const
	{
		std::shared_lock lock(m_MutexExperience);
		return m_Experience.has_value();
	}

	Experience Entity::GetExperience() const
	{
		std::shared_lock lock(m_MutexExperience);
		assert(m_Experience.has_value() && "Entity must have an Experience component to get it.");
		return *m_Experience;
	}

	void Entity::SetExperience(const Experience& experience)
	{
		std::unique_lock lock(m_MutexExperience);
		m_Experience = experience;
	}

	// ----- Inventory -----

	bool Entity::HasInventory() const
	{
		std::shared_lock lock(m_MutexInventory);
		return m_Inventory.has_value();
	}

	Inventory Entity::GetInventory() const
	{
		std::shared_lock lock(m_MutexInventory);
		assert(m_Inventory.has_value() && "Entity must have an Inventory component to get it.");
		return *m_Inventory;
	}

	void Entity::SetInventory(const Inventory& inventory)
	{
		std::unique_lock lock(m_MutexInventory);
		m_Inventory = inventory;
	}

	// ----- Hotbar -----

	bool Entity::HasHotbar() const
	{
		std::shared_lock lock(m_MutexHotbar);
		return m_Hotbar.has_value();
	}

	Hotbar Entity::GetHotbar() const
	{
		std::shared_lock lock(m_MutexHotbar);
		assert(m_Hotbar.has_value() && "Entity must have a Hotbar component to get it.");
		return *m_Hotbar;
	}

	void Entity::SetHotbar(const Hotbar& hotbar)
	{
		std::unique_lock lock(m_MutexHotbar);
		m_Hotbar = hotbar;
	}

} // namespace onion::voxel
