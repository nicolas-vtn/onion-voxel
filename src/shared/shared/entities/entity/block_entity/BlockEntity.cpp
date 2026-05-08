#include "BlockEntity.hpp"

#include <mutex>

namespace onion::voxel
{
	BlockEntity::BlockEntity(const std::string& uuid) : Entity(EntityType::Block, uuid)
	{
		SetTransform(Transform{});

		PhysicsBody physicsBody;
		physicsBody.Size = Size;
		physicsBody.CenterOffset = glm::vec3(0.f, Size.y * 0.5f, 0.f);
		physicsBody.Mass = 0.1f;
		physicsBody.IsFlying = false;
		SetPhysicsBody(physicsBody);
	}

	Slot BlockEntity::GetSlot() const
	{
		std::shared_lock lock(m_MutexBlock);
		return m_Slot;
	}

	void BlockEntity::SetSlot(const Slot& slot)
	{
		std::unique_lock lock(m_MutexBlock);
		m_Slot = slot;
	}

	float BlockEntity::GetLifetime() const
	{
		std::shared_lock lock(m_MutexBlock);
		return m_Lifetime;
	}

	void BlockEntity::SetLifetime(float seconds)
	{
		std::unique_lock lock(m_MutexBlock);
		m_Lifetime = seconds;
	}

	bool BlockEntity::IsExpired() const
	{
		std::shared_lock lock(m_MutexBlock);
		return m_Lifetime <= 0.f;
	}

	void BlockEntity::DecrementLifetime(float delta)
	{
		std::unique_lock lock(m_MutexBlock);
		m_Lifetime -= delta;
		if (m_Lifetime < 0.f)
			m_Lifetime = 0.f;
	}

	bool BlockEntity::CanBePickedUp() const
	{
		std::shared_lock lock(m_MutexBlock);
		return m_PickupCooldown <= 0.f;
	}

	void BlockEntity::DecrementPickupCooldown(float delta)
	{
		std::unique_lock lock(m_MutexBlock);
		m_PickupCooldown -= delta;
		if (m_PickupCooldown < 0.f)
			m_PickupCooldown = 0.f;
	}

} // namespace onion::voxel
