#pragma once

#include <renderer/Renderer.hpp>

namespace onion::voxel
{
	class Client
	{
	  public:
		void SayHello();

		void Start();
		void Stop();
		void Wait();
		bool IsRunning() const noexcept;

	  private:
		Renderer m_Renderer;
	};

} // namespace onion::voxel
