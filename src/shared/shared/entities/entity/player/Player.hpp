#pragma once

#include <mutex>

#include <shared/entities/entity/Entity.hpp>

namespace onion::voxel
{
	class Player : public Entity
	{
		// ------ Constructors & Destructor ------
	  public:
		Player(const std::string& uuid);
		~Player() override = default;

		// ------ Public API ------
	  public:
		static constexpr glm::vec3 Size{0.6f, 1.875f, 0.6f};

		glm::vec3 GetEyePosition() const;

		// ------ Getters / Setters ------
	  public:
		const std::string GetName() const;
		void SetName(const std::string& name);

		bool IsSneaking() const;
		void SetIsSneaking(bool isSneaking);

		// ------ Private Members ------
	  private:
		mutable std::shared_mutex m_MutexPlayerMembers;
		std::string m_Name;
		bool m_IsSneaking = false;

		// ------ Static Members ------
	  private:
		static constexpr glm::vec3 s_EyeOffset{0.f, 1.625f, 0.f};
		static constexpr glm::vec3 s_CrouchEyeOffset{0.f, -0.2f, 0.f};
	};
} // namespace onion::voxel
