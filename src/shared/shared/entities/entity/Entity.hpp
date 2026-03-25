#pragma once

#include <glm/glm.hpp>

#include <string>

#include "EntityTypes.hpp"

namespace onion::voxel
{
	class Entity
	{
	  public:
		Entity(EntityType type, const std::string& uuid, const std::string& name);
		virtual ~Entity();

		EntityType GetType() const;

		const std::string& GetUUID() const;
		void SetUUID(const std::string& uuid);

		const std::string& GetName() const;
		void SetName(const std::string& name);

		const glm::vec3& GetPosition() const;
		void SetPosition(const glm::vec3& position);

		const glm::vec3& GetFacing() const;
		void SetFacing(const glm::vec3& facing);

	  protected:
		EntityType m_Type = EntityType::None;

		std::string m_UUID;
		std::string m_Name;

		glm::vec3 m_Position{};
		glm::vec3 m_Facing{1.f, 0.f, 0.f};
	};
} // namespace onion::voxel
