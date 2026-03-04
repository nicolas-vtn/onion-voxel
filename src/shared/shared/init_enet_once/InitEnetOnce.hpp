#pragma once

#include <enet/enet.h>

namespace onion::voxel
{
	class InitEnetOnce
	{
	  public:
		static inline void Init()
		{
			if (!initialized)
			{
				if (enet_initialize() != 0)
				{
					throw std::runtime_error("Failed to initialize ENet.");
				}
				initialized = true;
			}
		}

		static inline void Deinit()
		{
			if (initialized)
			{
				enet_deinitialize();
				initialized = false;
			}
		}

	  private:
		static inline bool initialized = false;
	};
} // namespace onion::voxel
