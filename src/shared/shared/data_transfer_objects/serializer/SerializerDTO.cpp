#include "SerializerDTO.hpp"

namespace onion::voxel
{
	SubChunkDTO SerializerDTO::SerializeSubChunk(const SubChunk& sc)
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
		std::vector<uint16_t> raw(arr.begin(), arr.end());

		// --------- RLE ---------
		std::vector<uint16_t> rle;
		rle.reserve(SIZE);

		uint16_t current = arr[0];
		uint16_t count = 1;

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

	SubChunk SerializerDTO::DeserializeSubChunk(const SubChunkDTO& dto)
	{
		SubChunk sc;

		sc.m_IsMonoBlock = dto.compressionType == SubChunkDTO::MonoIndex;
		sc.m_MonoBlockIndexInPalette = dto.monoIndex;

		if (sc.m_IsMonoBlock)
			return sc;

		sc.m_BlockIndexInPalette = std::make_shared<
			std::array<uint16_t,
					   WorldConstants::CHUNK_SIZE * WorldConstants::CHUNK_SIZE * WorldConstants::CHUNK_SIZE>>();

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
				uint16_t count = dto.rleData[i];
				uint16_t value = dto.rleData[i + 1];

				for (uint16_t c = 0; c < count; c++)
				{
					arr[writeIndex++] = value;
				}
			}
		}

		return sc;
	}

	ChunkDTO SerializerDTO::SerializeChunk(std::shared_ptr<Chunk> chunk)
	{
		std::shared_lock lock(chunk->m_Mutex);

		ChunkDTO dto;
		dto.Position = chunk->GetPosition();

		dto.Palette.reserve(chunk->m_BlocksPalette.size());

		for (const BlockState& block : chunk->m_BlocksPalette)
		{
			dto.Palette.emplace_back(SerializeBlockState(block));
		}

		dto.SubChunks.reserve(chunk->m_SubChunks.size());

		for (const SubChunk& sc : chunk->m_SubChunks)
		{
			dto.SubChunks.emplace_back(SerializeSubChunk(sc));
		}

		return dto;
	}

	std::shared_ptr<Chunk> SerializerDTO::DeserializeChunk(const ChunkDTO& dto)
	{
		auto chunk = std::make_shared<Chunk>(dto.Position);

		{
			std::unique_lock lock(chunk->m_Mutex);

			chunk->m_BlocksPalette.clear();

			for (const BlockStateDTO& blockStateDto : dto.Palette)
			{
				chunk->m_BlocksPalette.emplace_back(DeserializeBlockState(blockStateDto));
			}

			chunk->m_SubChunks.clear();

			for (const SubChunkDTO& subChunkDTO : dto.SubChunks)
			{
				chunk->m_SubChunks.emplace_back(DeserializeSubChunk(subChunkDTO));
			}
		}

		return chunk;
	};

	BlockStateDTO SerializerDTO::SerializeBlockState(const BlockState& block)
	{
		BlockStateDTO dto;
		dto.id = (uint16_t) block.ID;
		dto.variantIndex = block.VariantIndex;

		return dto;
	}

	BlockState SerializerDTO::DeserializeBlockState(const BlockStateDTO& dto)
	{
		BlockState block;
		block.ID = (BlockId) dto.id;
		block.VariantIndex = dto.variantIndex;

		return block;
	}

	BlockDTO SerializerDTO::SerializeBlock(const Block& block)
	{
		BlockDTO dto;
		dto.position = block.Position;
		dto.state = SerializeBlockState(block.State);

		return dto;
	}

	Block SerializerDTO::DeserializeBlock(const BlockDTO& dto)
	{
		Block block;
		block.Position = dto.position;
		block.State = DeserializeBlockState(dto.state);

		return block;
	}

	OutOfBoundsBlocksDTO SerializerDTO::SerializeOutOfBoundsBlocks(
		const std::unordered_map<glm::ivec2, std::vector<Block>>& outOfBoundsBlocks)
	{
		OutOfBoundsBlocksDTO dto;
		for (const auto& [chunkPos, blocks] : outOfBoundsBlocks)
		{
			std::vector<BlockDTO> blockDTOs;
			blockDTOs.reserve(blocks.size());
			for (const Block& block : blocks)
			{
				blockDTOs.emplace_back(SerializeBlock(block));
			}
			dto.OutOfBoundsBlocks[chunkPos] = std::move(blockDTOs);
		}
		return dto;
	}

	std::unordered_map<glm::ivec2, std::vector<Block>>
	SerializerDTO::DeserializeOutOfBoundsBlocks(const OutOfBoundsBlocksDTO& dto)
	{
		std::unordered_map<glm::ivec2, std::vector<Block>> outOfBoundsBlocks;
		for (const auto& [chunkPos, blockDTOs] : dto.OutOfBoundsBlocks)
		{
			std::vector<Block> blocks;
			blocks.reserve(blockDTOs.size());
			for (const BlockDTO& blockDTO : blockDTOs)
			{
				blocks.emplace_back(DeserializeBlock(blockDTO));
			}
			outOfBoundsBlocks[chunkPos] = std::move(blocks);
		}
		return outOfBoundsBlocks;
	}

	TransformDTO SerializerDTO::SerializeTransform(const Transform& transform)
	{
		TransformDTO dto;
		dto.Position = transform.Position;
		dto.Rotation = transform.Rotation;
		dto.Scale = transform.Scale;
		return dto;
	}

	Transform SerializerDTO::DeserializeTransform(const TransformDTO& dto)
	{
		Transform transform;
		transform.Position = dto.Position;
		transform.Rotation = dto.Rotation;
		transform.Scale = dto.Scale;
		return transform;
	}

	PhysicsBodyDTO SerializerDTO::SerializePhysicsBody(const PhysicsBody& physicsBody)
	{
		PhysicsBodyDTO dto;
		dto.Velocity = physicsBody.Velocity;
		dto.OnGround = physicsBody.OnGround;
		dto.IsFlying = physicsBody.IsFlying;
		dto.HalfSize = physicsBody.HalfSize;
		dto.Offset = physicsBody.Offset;
		return dto;
	}

	PhysicsBody SerializerDTO::DeserializePhysicsBody(const PhysicsBodyDTO& dto)
	{
		PhysicsBody physicsBody;
		physicsBody.Velocity = dto.Velocity;
		physicsBody.OnGround = dto.OnGround;
		physicsBody.IsFlying = dto.IsFlying;
		physicsBody.HalfSize = dto.HalfSize;
		physicsBody.Offset = dto.Offset;
		return physicsBody;
	}

	void SerializerDTO::ApplyEntityDTO(const EntityDTO& dto, std::shared_ptr<Entity> entity)
	{
		entity->SetState(static_cast<Entity::State>(dto.State));

		if (dto.PhysicsBody)
		{
			entity->SetPhysicsBody(DeserializePhysicsBody(*dto.PhysicsBody));
		}

		if (dto.Transform)
		{
			entity->SetTransform(DeserializeTransform(*dto.Transform));
		}
	}

	EntityDTO SerializerDTO::SerializeEntity(const Entity& entity)
	{
		EntityDTO dto;
		dto.Type = static_cast<int>(entity.Type);
		dto.UUID = entity.UUID;
		dto.State = static_cast<uint8_t>(entity.GetState());
		if (entity.HasPhysicsBody())
			dto.PhysicsBody = SerializePhysicsBody(entity.GetPhysicsBody());
		if (entity.HasTransform())
			dto.Transform = SerializeTransform(entity.GetTransform());
		return dto;
	}

	std::shared_ptr<Entity> SerializerDTO::DeserializeEntity(const EntityDTO& dto)
	{
		auto entity = std::make_shared<Entity>(static_cast<EntityType>(dto.Type), dto.UUID);
		entity->SetState(static_cast<Entity::State>(dto.State));
		ApplyEntityDTO(dto, entity);

		return entity;
	}

	PlayerDTO SerializerDTO::SerializePlayer(const Player& player)
	{
		EntityDTO entityDTO = SerializeEntity(player);
		PlayerDTO playerDto(entityDTO);
		playerDto.Name = player.GetName();
		return playerDto;
	}

	std::shared_ptr<Player> SerializerDTO::DeserializePlayer(const PlayerDTO& dto)
	{
		auto player = std::make_shared<Player>(dto.UUID);
		ApplyEntityDTO(dto, player);

		player->SetName(dto.Name);

		return player;
	}

} // namespace onion::voxel
