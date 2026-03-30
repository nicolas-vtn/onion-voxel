#pragma once

#include <memory>

#include "NetworkMessages.hpp"

#include <shared/entities/entity/Entity.hpp>
#include <shared/entities/entity/player/Player.hpp>
#include <shared/world/chunk/Chunk.hpp>

#include <shared/network_messages/DTOs/DTOs.hpp>

namespace onion::voxel
{
	class SerializerDTO
	{
		// ----- CHUNK -----
	  public:
		static ChunkDataMsg SerializeChunk(std::shared_ptr<Chunk> chunk);
		static std::shared_ptr<Chunk> DeserializeChunk(const ChunkDataMsg& msg);

		// ----- SUB CHUNK -----
	  public:
		static SubChunkDTO SerializeSubChunk(const SubChunk& sc);
		static SubChunk DeserializeSubChunk(const SubChunkDTO& dto);

		// ----- BLOCK STATE -----
	  public:
		static BlockStateDTO SerializeBlockState(const BlockState& block);
		static BlockState DeserializeBlockState(const BlockStateDTO& dto);

		// ----- BLOCK -----
	  public:
		static BlockDTO SerializeBlock(const Block& block);
		static Block DeserializeBlock(const BlockDTO& dto);

		// ----- Entity -----
	  public:
		static TransformDTO SerializeTransform(const Transform& transform);
		static Transform DeserializeTransform(const TransformDTO& dto);
		static PhysicsBodyDTO SerializePhysicsBody(const PhysicsBody& physicsBody);
		static PhysicsBody DeserializePhysicsBody(const PhysicsBodyDTO& dto);

		static void ApplyEntityDTO(const EntityDTO& dto, std::shared_ptr<Entity> entity);
		static EntityDTO SerializeEntity(const Entity& entity);
		static std::shared_ptr<Entity> DeserializeEntity(const EntityDTO& dto);
		static PlayerDTO SerializePlayer(const Player& player);
		static std::shared_ptr<Player> DeserializePlayer(const PlayerDTO& dto);
	};
} // namespace onion::voxel
