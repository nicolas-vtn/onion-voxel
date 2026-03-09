#pragma once

#include <chrono>

namespace onion::voxel
{
	class Stopwatch
	{
		// ----- Public API -----
	  public:
		void Start() { m_Start = Clock::now(); }
		double ElapsedMs() const { return std::chrono::duration<double, std::milli>(Clock::now() - m_Start).count(); }
		double ElapsedSeconds() const { return std::chrono::duration<double>(Clock::now() - m_Start).count(); }

		// ----- Private Members -----
	  private:
		using Clock = std::chrono::high_resolution_clock;
		std::chrono::time_point<Clock> m_Start;
	};
} // namespace onion::voxel
