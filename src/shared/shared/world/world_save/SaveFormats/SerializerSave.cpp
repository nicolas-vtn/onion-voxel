#include "SerializerSave.hpp"

namespace onion::voxel
{
	std::vector<uint8_t> SerializerSave::SerializeChunk(const std::shared_ptr<Chunk>& chunk)
	{
		if (!chunk)
		{
			return {};
		}

		ChunkSave dto = SerializeChunkToSave(chunk);

		std::ostringstream oss(std::ios::binary);
		cereal::BinaryOutputArchive archive(oss);
		archive(dto);

		const std::string& buffer = oss.str();
		return std::vector<uint8_t>(buffer.begin(), buffer.end());
	}

	std::shared_ptr<Chunk> SerializerSave::DeserializeChunk(const std::vector<uint8_t>& data)
	{
		if (data.empty())
		{
			return nullptr;
		}

		ChunkSave dto;

		std::istringstream iss(std::string(reinterpret_cast<const char*>(data.data()), data.size()), std::ios::binary);

		cereal::BinaryInputArchive archive(iss);
		archive(dto);

		return DeserializeChunk(dto);
	}

	ChunkSave SerializerSave::SerializeChunkToSave(const std::shared_ptr<Chunk>& chunk)
	{
		std::shared_lock lock(chunk->m_Mutex);

		ChunkSave msg;
		msg.Position = chunk->GetPosition();

		msg.Palette.reserve(chunk->m_BlocksPalette.size());

		for (const BlockState& block : chunk->m_BlocksPalette)
		{
			msg.Palette.emplace_back(SerializeBlockState(block));
		}

		msg.SubChunks.reserve(chunk->m_SubChunks.size());

		for (const SubChunk& sc : chunk->m_SubChunks)
		{
			msg.SubChunks.emplace_back(SerializeSubChunk(sc));
		}

		return msg;
	}

	std::shared_ptr<Chunk> SerializerSave::DeserializeChunk(const ChunkSave& dto)
	{
		auto chunk = std::make_shared<Chunk>(dto.Position);

		{
			std::unique_lock lock(chunk->m_Mutex);

			chunk->m_BlocksPalette.clear();

			for (const BlockStateSave& dtoBlockState : dto.Palette)
			{
				chunk->m_BlocksPalette.emplace_back(DeserializeBlockState(dtoBlockState));
			}

			chunk->m_SubChunks.clear();

			for (const SubChunkSave& dtoSubChunk : dto.SubChunks)
			{
				chunk->m_SubChunks.emplace_back(DeserializeSubChunk(dtoSubChunk));
			}
		}

		return chunk;
	}

	SubChunkSave SerializerSave::SerializeSubChunk(const SubChunk& sc)
	{
		SubChunkSave dto;

		if (sc.m_IsMonoBlock)
		{
			dto.compressionType = SubChunkSave::MonoIndex;
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
			dto.compressionType = SubChunkSave::RLE;
			dto.rleData = std::move(rle);
		}
		else
		{
			dto.compressionType = SubChunkSave::None;
			dto.indices = std::move(raw);
		}

		return dto;
	}

	SubChunk SerializerSave::DeserializeSubChunk(const SubChunkSave& dto)
	{
		SubChunk sc;

		sc.m_IsMonoBlock = dto.compressionType == SubChunkSave::MonoIndex;
		sc.m_MonoBlockIndexInPalette = dto.monoIndex;

		if (sc.m_IsMonoBlock)
			return sc;

		sc.m_BlockIndexInPalette = std::make_shared<
			std::array<uint8_t,
					   WorldConstants::CHUNK_SIZE * WorldConstants::CHUNK_SIZE * WorldConstants::CHUNK_SIZE>>();

		auto& arr = *sc.m_BlockIndexInPalette;

		if (dto.compressionType == SubChunkSave::None)
		{
			std::copy(dto.indices.begin(), dto.indices.end(), arr.begin());
		}
		else if (dto.compressionType == SubChunkSave::RLE)
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

	BlockStateSave SerializerSave::SerializeBlockState(const BlockState& block)
	{
		BlockStateSave dto;
		dto.id = (uint16_t) block.ID;
		dto.facing = (uint8_t) block.Facing;
		dto.top = (uint8_t) block.Top;

		return dto;
	}

	BlockState SerializerSave::DeserializeBlockState(const BlockStateSave& dto)
	{
		BlockState block;
		block.ID = (BlockId) dto.id;
		block.Facing = (BlockState::Orientation) dto.facing;
		block.Top = (BlockState::Orientation) dto.top;

		return block;
	}

} // namespace onion::voxel
