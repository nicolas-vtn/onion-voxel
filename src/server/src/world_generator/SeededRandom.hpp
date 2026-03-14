#pragma once

#include <bit>
#include <cstdint>
#include <glm/glm.hpp>

class SeededRandom
{
  public:
	void SetSeed(uint32_t seed) { m_Seed = seed; }

	double GetValue(double x) const
	{
		uint64_t xi = std::bit_cast<uint64_t>(x);
		uint64_t h = Hash(xi ^ m_Seed);
		return ToUnitDouble(h);
	}

	double GetValue(int x) const
	{
		uint64_t h = Hash(uint64_t(uint32_t(x)) ^ m_Seed);
		return ToUnitDouble(h);
	}

	double GetValue(glm::ivec2 v) const
	{
		uint64_t combined = (uint64_t(uint32_t(v.x)) << 32) | uint32_t(v.y);

		uint64_t h = Hash(combined ^ m_Seed);
		return ToUnitDouble(h);
	}

	double GetValue(glm::ivec3 v) const
	{
		uint64_t h = Hash(uint64_t(uint32_t(v.x)) ^ m_Seed);
		h = Hash(h ^ uint64_t(uint32_t(v.y)));
		h = Hash(h ^ uint64_t(uint32_t(v.z)));
		return ToUnitDouble(h);
	}

  private:
	uint32_t m_Seed = 0;

	static uint64_t Hash(uint64_t x)
	{
		x += 0x9e3779b97f4a7c15;
		x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9;
		x = (x ^ (x >> 27)) * 0x94d049bb133111eb;
		x ^= (x >> 31);
		return x;
	}

	static double ToUnitDouble(uint64_t h)
	{
		constexpr double inv = 1.0 / double(UINT64_MAX);
		return h * inv; // [0,1)
	}
};
