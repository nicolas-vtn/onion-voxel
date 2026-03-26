#pragma once

#include <glm/glm.hpp>

#include <optional>
#include <shared_mutex>
#include <string>

#include "EntityTypes.hpp"

#include <shared/entities/components/PhysicsBody.hpp>
#include <shared/entities/components/Transform.hpp>

namespace onion::voxel
{
	class Entity
	{
	  public:
		Entity(EntityType type, const std::string& uuid);
		virtual ~Entity();

		const EntityType Type;
		const std::string UUID;

		// ----- Getters / Setters -----
	  public:
		bool HasPhysicsBody() const;
		PhysicsBody GetPhysicsBody() const;
		void SetPhysicsBody(const PhysicsBody& physicsBody);

		bool HasTransform() const;
		Transform GetTransform() const;
		void SetTransform(const Transform& transform);

		glm::vec3 GetPosition() const;
		void SetPosition(const glm::vec3& position);

		glm::vec3 GetFacing() const;
		void SetFacing(const glm::vec3& facing);

		// ----- Private Members -----
	  private:
		mutable std::shared_mutex m_MutexPhysicsBody;
		std::optional<PhysicsBody> m_PhysicsBody;

		mutable std::shared_mutex m_MutexTransform;
		std::optional<Transform> m_Transform;
	};
} // namespace onion::voxel
