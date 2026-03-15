#pragma once

#include <cassert>
#include <glm/glm.hpp>

namespace onion::voxel
{
	enum class Biome : uint8_t
	{
		Ocean,
		Plains,
		Forest,
		Desert,
		Mountain,
		Snow,

		Count,
	};

	//static inline Biome GetBiome(float temperature, float humidity)
	//{
	//	assert(temperature >= -1.0f && temperature <= 1.0f);
	//	assert(humidity >= -1.0f && humidity <= 1.0f);

	//	//int t = (temperature + 1.0f); // 0..2
	//	//int h = (humidity + 1.0f);	  // 0..2

	//	//t = glm::clamp(t, 0, 1);
	//	//h = glm::clamp(h, 0, 1);

	//	//static const Biome table[2][2] = {{Biome::Mountain, Biome::Plains}, // -----
	//	//								  {Biome::Desert, Biome::Forest}};

	//	//return table[t][h];

	//	int t = (temperature + 1.0f) * 1.5f; // 0..2
	//	int h = (humidity + 1.0f) * 1.5f;	 // 0..2

	//	t = glm::clamp(t, 0, 2);
	//	h = glm::clamp(h, 0, 2);

	//	static const Biome table[3][3] = {{Biome::Snow, Biome::Taiga, Biome::SnowForest},
	//									  {Biome::Plains, Biome::Forest, Biome::Swamp},
	//									  {Biome::Desert, Biome::Savanna, Biome::Jungle}};

	//	return table[t][h];
	//}

} // namespace onion::voxel
