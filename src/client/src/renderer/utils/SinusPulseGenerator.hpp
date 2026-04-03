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

		float GetValuePulse(float timeSeconds) const
		{
			float t = std::fmod(timeSeconds * m_FrequencyHz, 1.0f); // 0 → 1

			// Smoothstep (ease in/out)
			float smooth = t * t * (3.0f - 2.0f * t);

			return m_Amplitude * smooth + m_ConstantOffset;
		}

		float GetValuePingPong(float timeSeconds) const
		{
			float t = std::fmod(timeSeconds * m_FrequencyHz, 1.0f); // 0 → 1

			float pingpong = t < 0.5f ? t * 2.0f : (1.0f - t) * 2.0f;

			return m_Amplitude * pingpong + m_ConstantOffset;
		}

		float GetValueSharpPulse(float timeSeconds) const
		{
			float t = std::fmod(timeSeconds * m_FrequencyHz, 1.0f);

			// Exponential ease (fast rise, slow decay)
			float pulse = std::pow(t, 0.3f);

			return m_Amplitude * pulse + m_ConstantOffset;
		}

		float GetValueSmoothPulse(float timeSeconds) const
		{
			float t = std::fmod(timeSeconds * m_FrequencyHz, 1.0f);

			// Ping-pong
			float p = t < 0.5f ? t * 2.0f : (1.0f - t) * 2.0f;

			// Smooth it
			float smooth = p * p * (3.0f - 2.0f * p);

			return m_Amplitude * smooth + m_ConstantOffset;
		}

	  private:
		float m_FrequencyHz;
		float m_Amplitude;
		float m_ConstantOffset;
		float m_PhaseOffset;
	};
} // namespace onion::voxel
