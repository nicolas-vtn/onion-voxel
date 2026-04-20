#pragma once

#include "Camera.hpp"

#include <memory>
#include <shared_mutex>

namespace onion::voxel
{
	class FovSmoother
	{
		// ----- Constructor / Destructor -----
	  public:
		FovSmoother(std::shared_ptr<Camera> camera)
			: m_Camera(camera), m_TargetFov(camera->GetFov()), m_SmoothTime(0.05f), m_CurrentFov(camera->GetFov())
		{
		}

		~FovSmoother() = default;

		// ----- Public API -----
	  public:
		void Update(float deltaTime)
		{
			std::unique_lock<std::shared_mutex> lock(m_Mutex);
			if (m_TargetFov != m_CurrentFov)
			{

				// Smoothing factor based on exponential decay
				float lambda = 1.0f / m_SmoothTime;
				float alpha = 1.0f - std::exp(-lambda * deltaTime);

				m_CurrentFov += (m_TargetFov - m_CurrentFov) * alpha;
				m_Camera->SetFov(m_CurrentFov);

				// If we're very close to the target, snap to it to avoid endless small adjustments
				constexpr float epsilon = 0.001f;
				if (std::abs(m_TargetFov - m_CurrentFov) < epsilon)
				{
					m_CurrentFov = m_TargetFov;
				}
			}
		}

		// ----- Internal State -----
	  private:
		std::shared_ptr<Camera> m_Camera; // Reference to the camera whose FOV we want to smooth

		mutable std::shared_mutex m_Mutex; // Mutex to protect access to the FOV values
		float m_TargetFov = 0.0f;		   // Target field of view
		float m_SmoothTime = 0.05f;		   // Time to smooth the transition
		float m_CurrentFov = 0.0f;		   // Current field of view

		// ---------- GETTERS AND SETTERS ----------
	  public:
		void SetTargetFov(float targetFov)
		{
			std::unique_lock<std::shared_mutex> lock(m_Mutex);
			m_TargetFov = targetFov;
		}

		float GetTargetFov() const
		{
			std::shared_lock<std::shared_mutex> lock(m_Mutex);
			return m_TargetFov;
		}

		void SetSmoothTime(float smoothTime)
		{
			std::unique_lock<std::shared_mutex> lock(m_Mutex);
			m_SmoothTime = smoothTime;
		}

		float GetSmoothTime() const
		{
			std::shared_lock<std::shared_mutex> lock(m_Mutex);
			return m_SmoothTime;
		}
	};
} // namespace onion::voxel
