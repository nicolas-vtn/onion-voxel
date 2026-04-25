#include "Player.hpp"

namespace onion::voxel
{
	// ------ Constructors & Destructor ------
	Player::Player(const std::string& uuid) : Entity(EntityType::Player, uuid)
	{
		SetTransform(Transform{});

		PhysicsBody physicsBody;
		physicsBody.HalfSize = Size * 0.5f;
		physicsBody.Offset = glm::vec3(0.f, Size.y * 0.5f, 0.f);
		SetPhysicsBody(physicsBody);

		SetHotbar(Hotbar{});
		SetHealth(Health{20});
		SetHunger(Hunger{20});
		SetExperience(Experience{0});
		SetInventory(Inventory{});
	}

	glm::vec3 Player::GetEyePosition() const
	{
		glm::vec3 baseEyePosition = GetPosition() + s_EyeOffset;
		if (IsSneaking())
		{
			baseEyePosition += s_CrouchEyeOffset;
		}
		return baseEyePosition;
	}

	const std::string Player::GetName() const
	{
		std::shared_lock lock(m_MutexPlayerMembers);
		return m_Name;
	}

	void Player::SetName(const std::string& name)
	{
		std::unique_lock lock(m_MutexPlayerMembers);
		m_Name = name;
	}

	bool Player::IsSneaking() const
	{
		std::shared_lock lock(m_MutexPlayerMembers);
		return m_IsSneaking;
	}

	void Player::SetIsSneaking(bool isSneaking)
	{
		std::unique_lock lock(m_MutexPlayerMembers);
		m_IsSneaking = isSneaking;
	}

} // namespace onion::voxel
