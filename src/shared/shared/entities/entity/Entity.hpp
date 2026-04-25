#pragma once

#include <glm/glm.hpp>

#include <optional>
#include <shared_mutex>
#include <string>

#include "EntityTypes.hpp"

#include <shared/entities/components/Experience.hpp>
#include <shared/entities/components/Health.hpp>
#include <shared/entities/components/Hotbar.hpp>
#include <shared/entities/components/Hunger.hpp>
#include <shared/entities/components/Inventory.hpp>
#include <shared/entities/components/PhysicsBody.hpp>
#include <shared/entities/components/Transform.hpp>

namespace onion::voxel
{
	class Entity
	{
		// ----- Enums -----
	  public:
		enum class State : uint8_t
		{
			Idle = 0,
			Walking = 1,
			Running = 2,
			Jumping,
			Falling,
			Attacking,
			Dying
		};

		// ----- Constructor / Destructor -----
	  public:
		Entity(EntityType type, const std::string& uuid);
		virtual ~Entity();

		// ----- Public Members -----
	  public:
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

		bool HasHealth() const;
		Health GetHealth() const;
		void SetHealth(const Health& health);

		bool HasHunger() const;
		Hunger GetHunger() const;
		void SetHunger(const Hunger& hunger);

		bool HasExperience() const;
		Experience GetExperience() const;
		void SetExperience(const Experience& experience);

		bool HasInventory() const;
		Inventory GetInventory() const;
		void SetInventory(const Inventory& inventory);

		bool HasHotbar() const;
		Hotbar GetHotbar() const;
		void SetHotbar(const Hotbar& hotbar);

		glm::vec3 GetPosition() const;
		void SetPosition(const glm::vec3& position);

		glm::vec3 GetFacing() const;
		void SetFacing(const glm::vec3& facing);

		State GetState() const;
		void SetState(const State state);

		// ----- Private Members -----
	  private:
		mutable std::shared_mutex m_Mutex;
		State m_State = State::Idle;

		mutable std::shared_mutex m_MutexPhysicsBody;
		std::optional<PhysicsBody> m_PhysicsBody;

		mutable std::shared_mutex m_MutexTransform;
		std::optional<Transform> m_Transform;

		mutable std::shared_mutex m_MutexHealth;
		std::optional<Health> m_Health;

		mutable std::shared_mutex m_MutexHunger;
		std::optional<Hunger> m_Hunger;

		mutable std::shared_mutex m_MutexExperience;
		std::optional<Experience> m_Experience;

		mutable std::shared_mutex m_MutexInventory;
		std::optional<Inventory> m_Inventory;

		mutable std::shared_mutex m_MutexHotbar;
		std::optional<Hotbar> m_Hotbar;
	};
} // namespace onion::voxel
