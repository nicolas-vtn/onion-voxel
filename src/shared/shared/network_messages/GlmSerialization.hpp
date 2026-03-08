#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>

#include <glm/glm.hpp>

namespace cereal
{
	template <class Archive> void serialize(Archive& ar, glm::ivec2& v)
	{
		ar(v.x, v.y);
	}

	template <class Archive> void serialize(Archive& ar, glm::ivec3& v)
	{
		ar(v.x, v.y, v.z);
	}
} // namespace cereal
