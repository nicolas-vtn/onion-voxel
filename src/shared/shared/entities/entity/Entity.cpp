#include "Entity.hpp"

namespace onion::voxel
{
	Entity::Entity(EntityType type, const std::string& uuid, const std::string& name)
		: m_Type(type), m_UUID(uuid), m_Name(name)
	{
	}

	Entity::~Entity() = default;

	EntityType Entity::GetType() const
	{
		return m_Type;
	}

	const std::string& Entity::GetUUID() const
	{
		return m_UUID;
	}

	void Entity::SetUUID(const std::string& uuid)
	{
		m_UUID = uuid;
	}

	const std::string& Entity::GetName() const
	{
		return m_Name;
	}

	void Entity::SetName(const std::string& name)
	{
		m_Name = name;
	}

	const glm::vec3& Entity::GetPosition() const
	{
		return m_Position;
	}

	void Entity::SetPosition(const glm::vec3& position)
	{
		m_Position = position;
	}

	const glm::vec3& Entity::GetFacing() const
	{
		return m_Facing;
	}

	void Entity::SetFacing(const glm::vec3& facing)
	{
		m_Facing = facing;
	}

} // namespace onion::voxel
