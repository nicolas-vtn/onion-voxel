#include "WorldGenerator.hpp"

namespace onion::voxel
{
	WorldGenerator::WorldGenerator(std::shared_ptr<WorldManager> worldManager) : m_WorldManager(worldManager)
	{
		m_ThreadChunkGeneration = std::jthread([this](std::stop_token st) { ChunkGenerationThreadFunction(st); });
	}

	WorldGenerator::~WorldGenerator()
	{
		if (m_ThreadChunkGeneration.joinable())
		{
			m_ThreadChunkGeneration.request_stop();
			m_ThreadChunkGeneration.join();
		}
	}

	void WorldGenerator::GenerateChunkAsync(const glm::ivec2& chunkPosition)
	{
		m_ChunkGenerationQueue.Push(chunkPosition);
	}

	void WorldGenerator::GenerateChunksAsync(const std::vector<glm::ivec2>& chunkPositions)
	{
		for (const auto& chunkPosition : chunkPositions)
		{
			m_ChunkGenerationQueue.Push(chunkPosition);
		}
	}

	void WorldGenerator::ChunkGenerationThreadFunction(std::stop_token st)
	{
		while (!st.stop_requested())
		{
			glm::ivec2 chunkPosition;
			if (m_ChunkGenerationQueue.TryPop(chunkPosition))
			{
				if (!m_WorldManager->IsChunkLoaded(chunkPosition))
				{
					std::shared_ptr<Chunk> chunk = GenerateChunk(chunkPosition);
					if (chunk)
					{
						m_WorldManager->AddChunk(chunk);
					}
				}
			}
			else
			{
				std::this_thread::sleep_for(
					std::chrono::milliseconds(10)); // Sleep for a short duration to prevent busy waiting
			}
		}
	}

	std::shared_ptr<Chunk> WorldGenerator::GenerateChunk(const glm::ivec2& chunkPosition)
	{
		switch (m_WorldGenerationType)
		{
			case eWorldGenerationType::DemoBlocks:
				return GenerateChunk_DemoBlocks(chunkPosition);
			case eWorldGenerationType::Superflat:
				return GenerateChunk_Superflat(chunkPosition);
			case eWorldGenerationType::Classic:
				return GenerateChunk_Classic(chunkPosition);
			default:
				throw std::runtime_error("Invalid world generation type");
		}
	}

	std::shared_ptr<Chunk> WorldGenerator::GenerateChunk_DemoBlocks(const glm::ivec2& chunkPosition)
	{
		std::shared_ptr<Chunk> chunk = std::make_shared<Chunk>(chunkPosition);

		constexpr int spacing = 2; // 1 block + 1 air gap
		constexpr int start = 1;   // avoid borders

		// ---- All orientations except None ----
		std::vector<Block::Orientation> orientations = {Block::Orientation::Up,
														Block::Orientation::Down,
														Block::Orientation::North,
														Block::Orientation::South,
														Block::Orientation::East,
														Block::Orientation::West};

		// ---- Build valid (Facing, Top) pairs ----
		struct OrientationPair
		{
			Block::Orientation facing;
			Block::Orientation top;
		};

		std::vector<OrientationPair> orientationPairs;

		auto IsPerpendicular = [](Block::Orientation a, Block::Orientation b)
		{
			if (a == b)
				return false;

			auto opposite = [](Block::Orientation o)
			{
				switch (o)
				{
					case Block::Orientation::Up:
						return Block::Orientation::Down;
					case Block::Orientation::Down:
						return Block::Orientation::Up;
					case Block::Orientation::North:
						return Block::Orientation::South;
					case Block::Orientation::South:
						return Block::Orientation::North;
					case Block::Orientation::East:
						return Block::Orientation::West;
					case Block::Orientation::West:
						return Block::Orientation::East;
					default:
						return Block::Orientation::None;
				}
			};

			return opposite(a) != b;
		};

		for (auto facing : orientations)
			for (auto top : orientations)
				if (IsPerpendicular(facing, top))
					orientationPairs.push_back({facing, top});

		int x = start;
		int z = start;
		int y = start;

		const int max = WorldConstants::SUBCHUNK_SIZE - 1;

		// ---- Iterate all blocks ----
		const int blockIdCount = GetBlockIdCount();
		for (int blockId = 0; blockId < blockIdCount; ++blockId)
		{
			BlockId id = static_cast<BlockId>(blockId);

			if (id == BlockId::Air)
				continue;

			for (const auto& pair : orientationPairs)
			{
				if (x >= max)
				{
					x = start;
					z += spacing;
				}

				if (z >= max)
				{
					z = start;
					y += spacing;
				}

				if (y >= 100)
					return chunk;

				Block block{id};
				block.m_Facing = pair.facing;
				block.m_Top = pair.top;

				chunk->SetBlock({x, y, z}, block);

				x += spacing;
			}
		}

		return chunk;
	}

	std::shared_ptr<Chunk> WorldGenerator::GenerateChunk_Superflat(const glm::ivec2& chunkPosition)
	{
		std::shared_ptr<Chunk> chunk = std::make_shared<Chunk>(chunkPosition);

		int y = 0;

		// Bedrock layer
		Block bedrock{BlockId::Bedrock};
		for (y = 0; y <= 0; y++)
		{

			for (int x = 0; x < WorldConstants::SUBCHUNK_SIZE; x++)
			{
				for (int z = 0; z < WorldConstants::SUBCHUNK_SIZE; z++)
				{
					chunk->SetBlock(glm::ivec3(x, y, z), bedrock);
				}
			}
		}

		// 3 Dirt layers
		Block dirt{BlockId::Dirt};
		for (y = 1; y <= 3; y++)
		{
			for (int x = 0; x < WorldConstants::SUBCHUNK_SIZE; x++)
			{
				for (int z = 0; z < WorldConstants::SUBCHUNK_SIZE; z++)
				{
					chunk->SetBlock(glm::ivec3(x, y, z), dirt);
				}
			}
		}

		// Grass layer
		Block grass{BlockId::Grass};
		for (y = 4; y <= 4; y++)
		{
			for (int x = 0; x < WorldConstants::SUBCHUNK_SIZE; x++)
			{
				for (int z = 0; z < WorldConstants::SUBCHUNK_SIZE; z++)
				{
					chunk->SetBlock(glm::ivec3(x, y, z), grass);
				}
			}
		}

		return chunk;
	}

	std::shared_ptr<Chunk> WorldGenerator::GenerateChunk_Classic(const glm::ivec2& chunkPosition)
	{
		throw std::runtime_error("GenerateChunk_Classic is not implemented yet");
	}

} // namespace onion::voxel
