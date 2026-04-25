#pragma once

#include <memory>

#include <shared/entities/entity/Entity.hpp>
#include <shared/entities/entity/player/Player.hpp>
#include <shared/world/chunk/Chunk.hpp>

#include <shared/data_transfer_objects/DTOs/DTOs.hpp>

namespace onion::voxel
{
	class SerializerDTO
	{
		// ----- CHUNK -----
	  public:
		static ChunkDTO SerializeChunk(std::shared_ptr<Chunk> chunk);
		static std::shared_ptr<Chunk> DeserializeChunk(const ChunkDTO& dto);

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

		// ----- OUT OF BOUNDS BLOCKS -----
	  public:
		static OutOfBoundsBlocksDTO
		SerializeOutOfBoundsBlocks(const std::unordered_map<glm::ivec2, std::vector<Block>>& outOfBoundsBlocks);
		static std::unordered_map<glm::ivec2, std::vector<Block>>
		DeserializeOutOfBoundsBlocks(const OutOfBoundsBlocksDTO& dto);

		// ----- Entity -----
	  public:
		static TransformDTO SerializeTransform(const Transform& transform);
		static Transform DeserializeTransform(const TransformDTO& dto);
		static PhysicsBodyDTO SerializePhysicsBody(const PhysicsBody& physicsBody);
		static PhysicsBody DeserializePhysicsBody(const PhysicsBodyDTO& dto);

		// ----- Player Components -----
	  public:
		static HealthDTO SerializeHealth(const Health& health);
		static Health DeserializeHealth(const HealthDTO& dto);
		static HungerDTO SerializeHunger(const Hunger& hunger);
		static Hunger DeserializeHunger(const HungerDTO& dto);
		static ExperienceDTO SerializeExperience(const Experience& experience);
		static Experience DeserializeExperience(const ExperienceDTO& dto);
		static HotbarDTO SerializeHotbar(const Hotbar& hotbar);
		static Hotbar DeserializeHotbar(const HotbarDTO& dto);
		static InventoryDTO SerializeInventory(const Inventory& inventory);
		static Inventory DeserializeInventory(const InventoryDTO& dto);

		static void ApplyEntityDTO(const EntityDTO& dto, std::shared_ptr<Entity> entity);
		static EntityDTO SerializeEntity(const Entity& entity);
		static std::shared_ptr<Entity> DeserializeEntity(const EntityDTO& dto);
		static PlayerDTO SerializePlayer(const Player& player);
		static std::shared_ptr<Player> DeserializePlayer(const PlayerDTO& dto);
	};
} // namespace onion::voxel
