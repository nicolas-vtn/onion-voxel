#pragma once

#include <memory>

#include "NetworkMessages.hpp"

#include <shared/world/chunk/Chunk.hpp>

namespace onion::voxel
{
	class Serializer
	{
	  public:
		static inline ChunkDataMsg SerializeChunk(std::shared_ptr<Chunk> chunk)
		{
			std::shared_lock lock(chunk->m_Mutex);

			ChunkDataMsg msg;
			msg.Position = chunk->GetPosition();

			msg.Palette.reserve(chunk->m_BlocksPalette.size());

			for (const Block& block : chunk->m_BlocksPalette)
			{
				BlockDTO dto;
				dto.id = (uint16_t) block.m_BlockID;
				dto.facing = (uint8_t) block.m_Facing;
				dto.top = (uint8_t) block.m_Top;

				msg.Palette.push_back(dto);
			}

			msg.SubChunks.reserve(chunk->m_SubChunks.size());

			for (const SubChunk& sc : chunk->m_SubChunks)
			{
				SubChunkDTO dto;

				dto.isMono = sc.m_IsMonoBlock;
				dto.monoIndex = sc.m_MonoBlockIndexInPalette;

				if (!sc.m_IsMonoBlock)
				{
					dto.indices.assign(sc.m_BlockIndexInPalette->begin(), sc.m_BlockIndexInPalette->end());
				}

				msg.SubChunks.push_back(dto);
			}

			return msg;
		}

		static inline std::shared_ptr<Chunk> DeserializeChunk(const ChunkDataMsg& msg)
		{
			auto chunk = std::make_shared<Chunk>(msg.Position);

			{
				std::unique_lock lock(chunk->m_Mutex);

				chunk->m_BlocksPalette.clear();

				for (const BlockDTO& dto : msg.Palette)
				{
					Block block;
					block.m_BlockID = (BlockId) dto.id;
					block.m_Facing = (Block::Orientation) dto.facing;
					block.m_Top = (Block::Orientation) dto.top;

					chunk->m_BlocksPalette.push_back(block);
				}

				chunk->m_SubChunks.clear();

				for (const SubChunkDTO& dto : msg.SubChunks)
				{
					SubChunk sc;

					sc.m_IsMonoBlock = dto.isMono;
					sc.m_MonoBlockIndexInPalette = dto.monoIndex;

					if (!dto.isMono)
					{
						sc.m_BlockIndexInPalette =
							std::make_shared<std::array<uint8_t,
														WorldConstants::SUBCHUNK_SIZE * WorldConstants::SUBCHUNK_SIZE *
															WorldConstants::SUBCHUNK_SIZE>>();

						std::copy(dto.indices.begin(), dto.indices.end(), sc.m_BlockIndexInPalette->begin());
					}

					chunk->m_SubChunks.push_back(sc);
				}
			}

			return chunk;
		};
	};
} // namespace onion::voxel
