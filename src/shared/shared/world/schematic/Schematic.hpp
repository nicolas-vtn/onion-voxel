#pragma once

#include <glm/glm.hpp>
#include <stdexcept>
#include <vector>

#include "../block/Block.hpp"

namespace onion::voxel
{
	class Schematic
	{
	  public:
		Schematic(glm::ivec3 size, glm::ivec3 origin = {})
			: m_Size(size), m_Origin(origin), m_Blocks(size.x * size.y * size.z, Block(BlockId::Air))
		{
		}

		const glm::ivec3& GetSize() const { return m_Size; }
		const glm::ivec3& GetOrigin() const { return m_Origin; }

		void SetOrigin(glm::ivec3 origin) { m_Origin = origin; }

		Block GetBlock(glm::ivec3 pos) const
		{
			if (!IsInside(pos))
				return Block(BlockId::Air);

			return m_Blocks[Index(pos)];
		}

		BlockId GetBlockId(glm::ivec3 pos) const
		{
			if (!IsInside(pos))
				return BlockId::Air;

			return m_Blocks[Index(pos)].m_BlockID;
		}

		void SetBlock(glm::ivec3 pos, const Block& block)
		{
			if (!IsInside(pos))
				throw std::out_of_range("Schematic::SetBlock out of bounds");

			m_Blocks[Index(pos)] = block;
		}

		size_t GetBlockCount() const { return m_Blocks.size(); }

		void Fill(Block block) { std::fill(m_Blocks.begin(), m_Blocks.end(), block); }

		bool IsInside(glm::ivec3 pos) const
		{
			return pos.x >= 0 && pos.x < m_Size.x && pos.y >= 0 && pos.y < m_Size.y && pos.z >= 0 && pos.z < m_Size.z;
		}

	  private:
		size_t Index(glm::ivec3 pos) const { return pos.x + m_Size.x * (pos.z + m_Size.z * pos.y); }

	  private:
		glm::ivec3 m_Size;
		glm::ivec3 m_Origin;

		std::vector<Block> m_Blocks;
	};
} // namespace onion::voxel
