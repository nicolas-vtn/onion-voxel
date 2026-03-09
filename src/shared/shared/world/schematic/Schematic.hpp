#pragma once

#include <glm/glm.hpp>

#include <vector>

#include "../block/Block.hpp"

namespace onion::voxel
{
	class Schematic
	{
	  public:
		glm::ivec3 Origin; // Origin position of the schematic in the world

		int SizeX = 0; // Size in X direction
		int SizeY = 0; // Size in Y direction
		int SizeZ = 0; // Size in Z direction

		std::vector<std::vector<std::vector<Block>>> Blocks; // List of blocks in the schematic

		Schematic(int sizeX, int sizeY, int sizeZ, glm::ivec3 origin)
			: Origin(origin), SizeX(sizeX), SizeY(sizeY), SizeZ(sizeZ)
		{
			// Initialize the Blocks vector with Air blocks
			Blocks.resize(SizeX,
						  std::vector<std::vector<Block>>(SizeY, std::vector<Block>(SizeZ, Block(BlockId::Air))));
		}

		void Resize(int newSizeX, int newSizeY, int newSizeZ)
		{
			// Resize the Blocks vector to the new size
			Blocks.resize(newSizeX);
			for (auto& vecY : Blocks)
			{
				vecY.resize(newSizeY);
				for (auto& vecZ : vecY)
				{
					vecZ.resize(newSizeZ, Block(BlockId::Air));
				}
			}

			SizeX = newSizeX;
			SizeY = newSizeY;
			SizeZ = newSizeZ;
		}

		Block GetBlockAt(int x, int y, int z) const
		{
			if (x < 0 || x >= SizeX || y < 0 || y >= SizeY || z < 0 || z >= SizeZ)
			{
				return Block(BlockId::Air); // Return void block if out of bounds
			}
			return Blocks[x][y][z]; // Return the block at the specified position
		}

		BlockId GetBlockIdAt(int x, int y, int z) const
		{
			if (x < 0 || x >= SizeX || y < 0 || y >= SizeY || z < 0 || z >= SizeZ)
			{
				return BlockId::Air; // Return air block if out of bounds
			}
			return Blocks[x][y][z].m_BlockID; // Return the block at the specified position
		}

		void SetBlockAt(int x, int y, int z, const Block& block)
		{

			if (x < 0 || y < 0 || z < 0)
			{
				throw std::out_of_range("Coordinates must be non-negative");
			}

			if (x >= SizeX || y >= SizeY || z >= SizeZ)
			{
				this->Resize(std::max(SizeX, x + 1), std::max(SizeY, y + 1), std::max(SizeZ, z + 1));
			}

			Blocks[x][y][z] = block; // Set the block at the specified position
		}
	};
} // namespace onion::voxel
