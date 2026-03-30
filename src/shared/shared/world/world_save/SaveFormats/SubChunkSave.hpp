#pragma once

#include <cereal/archives/binary.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>

namespace onion::voxel
{
	struct SubChunkSave
	{
		enum eCompressionType : uint8_t
		{
			None = 0,	   // No compression, raw block indices in palette
			MonoIndex = 1, // All blocks are the same
			RLE = 2,	   // Run-Length Encoding
		};

		eCompressionType compressionType = eCompressionType::MonoIndex;

		uint8_t monoIndex{0};

		std::vector<uint8_t> indices; // Raw block indices in palette (compressionType is None)
		std::vector<uint8_t> rleData; // Run-Length Encoding (compressionType is RLE) (pairs of [count, indexInPalette])

		template <class Archive> void serialize(Archive& ar) { ar(compressionType, monoIndex, indices, rleData); }
	};
} // namespace onion::voxel
