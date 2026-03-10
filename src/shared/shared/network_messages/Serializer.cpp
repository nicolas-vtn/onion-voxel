#include "Serializer.hpp"

namespace onion::voxel
{
	SubChunkDTO Serializer::SerializeSubChunk(const SubChunk& sc)
	{
		SubChunkDTO dto;

		if (sc.m_IsMonoBlock)
		{
			dto.compressionType = SubChunkDTO::MonoIndex;
			dto.monoIndex = sc.m_MonoBlockIndexInPalette;
			return dto;
		}

		const auto& arr = *sc.m_BlockIndexInPalette;
		const size_t SIZE = arr.size();

		// --------- RAW ---------
		std::vector<uint8_t> raw(arr.begin(), arr.end());

		// --------- RLE ---------
		std::vector<uint8_t> rle;
		rle.reserve(SIZE);

		uint8_t current = arr[0];
		uint8_t count = 1;

		for (size_t i = 1; i < SIZE; i++)
		{
			if (arr[i] == current && count < 255)
			{
				count++;
			}
			else
			{
				rle.push_back(count);
				rle.push_back(current);

				current = arr[i];
				count = 1;
			}
		}

		rle.push_back(count);
		rle.push_back(current);

		// --------- Choose best compression ---------
		if (rle.size() < raw.size())
		{
			dto.compressionType = SubChunkDTO::RLE;
			dto.rleData = std::move(rle);
		}
		else
		{
			dto.compressionType = SubChunkDTO::None;
			dto.indices = std::move(raw);
		}

		return dto;
	}

	SubChunk Serializer::DeserializeSubChunk(const SubChunkDTO& dto)
	{
		SubChunk sc;

		sc.m_IsMonoBlock = dto.compressionType == SubChunkDTO::MonoIndex;
		sc.m_MonoBlockIndexInPalette = dto.monoIndex;

		if (sc.m_IsMonoBlock)
			return sc;

		sc.m_BlockIndexInPalette =
			std::make_shared<std::array<uint8_t,
										WorldConstants::CHUNK_SIZE * WorldConstants::CHUNK_SIZE *
											WorldConstants::CHUNK_SIZE>>();

		auto& arr = *sc.m_BlockIndexInPalette;

		if (dto.compressionType == SubChunkDTO::None)
		{
			std::copy(dto.indices.begin(), dto.indices.end(), arr.begin());
		}
		else if (dto.compressionType == SubChunkDTO::RLE)
		{
			size_t writeIndex = 0;

			for (size_t i = 0; i < dto.rleData.size(); i += 2)
			{
				uint8_t count = dto.rleData[i];
				uint8_t value = dto.rleData[i + 1];

				for (uint8_t c = 0; c < count; c++)
				{
					arr[writeIndex++] = value;
				}
			}
		}

		return sc;
	}

	ChunkDataMsg Serializer::SerializeChunk(std::shared_ptr<Chunk> chunk)
	{
		std::shared_lock lock(chunk->m_Mutex);

		ChunkDataMsg msg;
		msg.Position = chunk->GetPosition();

		msg.Palette.reserve(chunk->m_BlocksPalette.size());

		for (const Block& block : chunk->m_BlocksPalette)
		{
			msg.Palette.emplace_back(SerializeBlock(block));
		}

		msg.SubChunks.reserve(chunk->m_SubChunks.size());

		for (const SubChunk& sc : chunk->m_SubChunks)
		{
			msg.SubChunks.emplace_back(SerializeSubChunk(sc));
		}

		return msg;
	}

	std::shared_ptr<Chunk> Serializer::DeserializeChunk(const ChunkDataMsg& msg)
	{
		auto chunk = std::make_shared<Chunk>(msg.Position);

		{
			std::unique_lock lock(chunk->m_Mutex);

			chunk->m_BlocksPalette.clear();

			for (const BlockDTO& dto : msg.Palette)
			{
				chunk->m_BlocksPalette.emplace_back(DeserializeBlock(dto));
			}

			chunk->m_SubChunks.clear();

			for (const SubChunkDTO& dto : msg.SubChunks)
			{
				chunk->m_SubChunks.emplace_back(DeserializeSubChunk(dto));
			}
		}

		return chunk;
	};

	BlockDTO Serializer::SerializeBlock(const Block& block)
	{
		BlockDTO dto;
		dto.id = (uint16_t) block.m_BlockID;
		dto.facing = (uint8_t) block.m_Facing;
		dto.top = (uint8_t) block.m_Top;

		return dto;
	}

	Block Serializer::DeserializeBlock(const BlockDTO& dto)
	{
		Block block;
		block.m_BlockID = (BlockId) dto.id;
		block.m_Facing = (Block::Orientation) dto.facing;
		block.m_Top = (Block::Orientation) dto.top;

		return block;
	}

} // namespace onion::voxel
