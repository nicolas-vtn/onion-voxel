#pragma once

#include <cmath>

namespace onion::voxel
{
	class SinusPulseGenerator
	{
	  public:
		SinusPulseGenerator(float frequencyHz,
							float amplitude = 1.0f,
							float constantOffset = 0.0f,
							float phaseOffset = 0.0f)
			: m_FrequencyHz(frequencyHz), m_Amplitude(amplitude), m_ConstantOffset(constantOffset),
			  m_PhaseOffset(phaseOffset)
		{
		}

		float GetValue(float timeSeconds) const
		{
			static float twoPi = 2.0f * 3.14159265358979323846f;
			float angularFrequency = twoPi * m_FrequencyHz;
			return m_Amplitude * std::sin(angularFrequency * timeSeconds + m_PhaseOffset) + m_ConstantOffset;
		}

	  private:
		float m_FrequencyHz;
		float m_Amplitude;
		float m_ConstantOffset;
		float m_PhaseOffset;
	};
} // namespace onion::voxel
