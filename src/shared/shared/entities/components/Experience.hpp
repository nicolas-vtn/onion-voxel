#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace onion::voxel
{
	struct Experience
	{
		uint32_t Value = 0;

		struct LevelInfo
		{
			int Level = 0;
			uint32_t ExperienceForNextLevel = 0;
			uint32_t TotalExperienceForCurrentLevel = 0;
		};

	  private:
		// ----- XP required to go from level -> level+1 -----
		static uint32_t XPToNextLevel(int level)
		{
			if (level <= 15)
				return 2 * level + 7;
			else if (level <= 30)
				return 5 * level - 38;
			else
				return 9 * level - 158;
		}

		// ----- Total XP required to reach a given level -----
		static uint32_t TotalXPForLevel(int level)
		{
			if (level <= 16)
			{
				return level * level + 6 * level;
			}
			else if (level <= 31)
			{
				return static_cast<uint32_t>(2.5 * level * level - 40.5 * level + 360.0);
			}
			else
			{
				return static_cast<uint32_t>(4.5 * level * level - 162.5 * level + 2220.0);
			}
		}

		// ----- Compute level from total XP (inverse formulas) -----
		static int LevelFromXP(uint32_t xp)
		{
			if (xp <= 352)
			{
				return static_cast<int>(std::floor(std::sqrt(xp + 9.0) - 3.0));
			}
			else if (xp <= 1507)
			{
				return static_cast<int>(std::floor(81.0 / 10.0 + std::sqrt((2.0 / 5.0) * (xp - 7839.0 / 40.0))));
			}
			else
			{
				return static_cast<int>(std::floor(325.0 / 18.0 + std::sqrt((2.0 / 9.0) * (xp - 54215.0 / 72.0))));
			}
		}

	  public:
		inline LevelInfo GetLevel() const
		{
			LevelInfo info{};

			const int level = LevelFromXP(Value);
			const uint32_t xpAtLevelStart = TotalXPForLevel(level);
			const uint32_t xpToNext = XPToNextLevel(level);

			info.Level = level;
			info.TotalExperienceForCurrentLevel = Value - xpAtLevelStart;
			info.ExperienceForNextLevel = xpToNext;

			return info;
		}
	};
} // namespace onion::voxel
