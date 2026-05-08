#pragma once

#include <shared_mutex>

#include <shared/entities/components/Inventory.hpp>
#include <shared/entities/entity/Entity.hpp>

namespace onion::voxel
{
	class BlockEntity : public Entity
	{
		// ----- Constructor / Destructor -----
	  public:
		explicit BlockEntity(const std::string& uuid);
		~BlockEntity() override = default;

		// ----- Static Constants -----
	  public:
		static constexpr glm::vec3 Size{0.25f, 0.25f, 0.25f};
		static constexpr float DefaultLifetime = 300.f;      // 5 minutes
		static constexpr float DefaultPickupCooldown = 1.0f; // seconds before item can be picked up

		// ----- Public API -----
	  public:
		Slot GetSlot() const;
		void SetSlot(const Slot& slot);

		float GetLifetime() const;
		void SetLifetime(float seconds);

		// Returns true if lifetime has reached zero.
		bool IsExpired() const;

		// Subtract delta seconds from remaining lifetime.
		void DecrementLifetime(float delta);

		// Returns true if the pickup cooldown has elapsed.
		bool CanBePickedUp() const;

		// Subtract delta seconds from the pickup cooldown.
		void DecrementPickupCooldown(float delta);

		// ----- Private Members -----
	  private:
		mutable std::shared_mutex m_MutexBlock;
		Slot m_Slot{};
		float m_Lifetime = DefaultLifetime;
		float m_PickupCooldown = DefaultPickupCooldown;
	};
} // namespace onion::voxel
