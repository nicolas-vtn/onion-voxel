#pragma once

#include <shared/entities/entity/Entity.hpp>

namespace onion::voxel
{
	class Player : public Entity
	{
		// ------ Public API ------
	  public:
		glm::vec3 GetEyePosition() const { return GetPosition() + s_EyeOffset; }

		static constexpr glm::vec3 Size{0.6f, 1.875f, 0.6f};

		// ------ Getters / Setters ------
	  public:
		const std::string GetName() const
		{
			std::shared_lock lock(m_MutexPlayerMembers);
			return Name;
		}

		void SetName(const std::string& name)
		{
			std::unique_lock lock(m_MutexPlayerMembers);
			Name = name;
		}

		// ------ Constructors & Destructor ------
	  public:
		Player(const std::string& uuid) : Entity(EntityType::Player, uuid)
		{
			SetTransform(Transform{});

			PhysicsBody physicsBody;
			physicsBody.HalfSize = Size * 0.5f;
			physicsBody.Offset = glm::vec3(0.f, Size.y * 0.5f, 0.f);
			SetPhysicsBody(physicsBody);
		}

		~Player() override {}

		// ------ Private Members ------
	  private:
		mutable std::shared_mutex m_MutexPlayerMembers;
		std::string Name;

		// ------ Static Members ------
	  private:
		static constexpr glm::vec3 s_EyeOffset{0.f, 1.625f, 0.f};
	};
} // namespace onion::voxel
