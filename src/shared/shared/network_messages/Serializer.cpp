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

		sc.m_BlockIndexInPalette = std::make_shared<
			std::array<uint8_t,
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

	std::shared_ptr<Chunk> Serializer::DeserializeChunk(const ChunkDataMsg& msg)
	{
		auto chunk = std::make_shared<Chunk>(msg.Position);

		{
			std::unique_lock lock(chunk->m_Mutex);

			chunk->m_BlocksPalette.clear();

			for (const BlockStateDTO& dto : msg.Palette)
			{
				chunk->m_BlocksPalette.emplace_back(DeserializeBlockState(dto));
			}

			chunk->m_SubChunks.clear();

			for (const SubChunkDTO& dto : msg.SubChunks)
			{
				chunk->m_SubChunks.emplace_back(DeserializeSubChunk(dto));
			}
		}

		return chunk;
	};

	BlockStateDTO Serializer::SerializeBlockState(const BlockState& block)
	{
		BlockStateDTO dto;
		dto.id = (uint16_t) block.ID;
		dto.facing = (uint8_t) block.Facing;
		dto.top = (uint8_t) block.Top;

		return dto;
	}

	BlockState Serializer::DeserializeBlockState(const BlockStateDTO& dto)
	{
		BlockState block;
		block.ID = (BlockId) dto.id;
		block.Facing = (BlockState::Orientation) dto.facing;
		block.Top = (BlockState::Orientation) dto.top;

		return block;
	}

	BlockDTO Serializer::SerializeBlock(const Block& block)
	{
		BlockDTO dto;
		dto.position = block.Position;
		dto.state = SerializeBlockState(block.State);

		return dto;
	}

	Block Serializer::DeserializeBlock(const BlockDTO& dto)
	{
		Block block;
		block.Position = dto.position;
		block.State = DeserializeBlockState(dto.state);

		return block;
	}

	TransformDTO Serializer::SerializeTransform(const Transform& transform)
	{
		TransformDTO dto;
		dto.Position = transform.Position;
		dto.Rotation = transform.Rotation;
		dto.Scale = transform.Scale;
		return dto;
	}

	Transform Serializer::DeserializeTransform(const TransformDTO& dto)
	{
		Transform transform;
		transform.Position = dto.Position;
		transform.Rotation = dto.Rotation;
		transform.Scale = dto.Scale;
		return transform;
	}

	PhysicsBodyDTO Serializer::SerializePhysicsBody(const PhysicsBody& physicsBody)
	{
		PhysicsBodyDTO dto;
		dto.Velocity = physicsBody.Velocity;
		dto.OnGround = physicsBody.OnGround;
		dto.IsFlying = physicsBody.IsFlying;
		dto.HalfSize = physicsBody.HalfSize;
		dto.Offset = physicsBody.Offset;
		return dto;
	}

	PhysicsBody Serializer::DeserializePhysicsBody(const PhysicsBodyDTO& dto)
	{
		PhysicsBody physicsBody;
		physicsBody.Velocity = dto.Velocity;
		physicsBody.OnGround = dto.OnGround;
		physicsBody.IsFlying = dto.IsFlying;
		physicsBody.HalfSize = dto.HalfSize;
		physicsBody.Offset = dto.Offset;
		return physicsBody;
	}

	void Serializer::ApplyEntityDTO(const EntityDTO& dto, std::shared_ptr<Entity> entity)
	{
		if (dto.PhysicsBody)
		{
			entity->SetPhysicsBody(DeserializePhysicsBody(*dto.PhysicsBody));
		}

		if (dto.Transform)
		{
			entity->SetTransform(DeserializeTransform(*dto.Transform));
		}
	}

	EntityDTO Serializer::SerializeEntity(const Entity& entity)
	{
		EntityDTO dto;
		dto.Type = static_cast<int>(entity.Type);
		dto.UUID = entity.UUID;
		if (entity.HasPhysicsBody())
			dto.PhysicsBody = SerializePhysicsBody(entity.GetPhysicsBody());
		if (entity.HasTransform())
			dto.Transform = SerializeTransform(entity.GetTransform());
		return dto;
	}

	std::shared_ptr<Entity> Serializer::DeserializeEntity(const EntityDTO& dto)
	{
		auto entity = std::make_shared<Entity>(static_cast<EntityType>(dto.Type), dto.UUID);
		ApplyEntityDTO(dto, entity);

		return entity;
	}

	PlayerDTO Serializer::SerializePlayer(const Player& player)
	{
		EntityDTO entityDTO = SerializeEntity(player);
		PlayerDTO playerDto(entityDTO);
		playerDto.Name = player.GetName();
		return playerDto;
	}

	std::shared_ptr<Player> Serializer::DeserializePlayer(const PlayerDTO& dto)
	{
		auto player = std::make_shared<Player>(dto.UUID);
		ApplyEntityDTO(dto, player);

		player->SetName(dto.Name);

		return player;
	}

} // namespace onion::voxel
