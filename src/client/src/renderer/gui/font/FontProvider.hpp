#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

namespace onion::voxel
{
	class FontProvider
	{
		// ----- Structs -----
	  public:
		struct GlyphPos
		{
			int rowIndex;
			int colIndex;
		};

		// ----- Constructor / Destructor -----
	  public:
		FontProvider() = default;
		~FontProvider() = default;

		// ----- Members -----
	  public:
		std::string file;
		std::unordered_map<char32_t, GlyphPos> chars;

		int totalRows = 0, totalCols = 0;
		int ascent = 7;
	};

	// Simple UTF-8 → UTF-32 conversion
	static std::u32string Utf8ToUtf32(const std::string& str)
	{
		std::u32string result;

		size_t i = 0;
		while (i < str.size())
		{
			uint32_t codepoint = 0;
			unsigned char c = static_cast<unsigned char>(str[i]);

			if (c <= 0x7F) // 1 byte
			{
				codepoint = c;
				i += 1;
			}
			else if ((c >> 5) == 0x6) // 2 bytes
			{
				if (i + 1 >= str.size())
					break; // safety

				codepoint = ((c & 0x1F) << 6) | (static_cast<unsigned char>(str[i + 1]) & 0x3F);
				i += 2;
			}
			else if ((c >> 4) == 0xE) // 3 bytes
			{
				if (i + 2 >= str.size())
					break;

				codepoint = ((c & 0x0F) << 12) | ((static_cast<unsigned char>(str[i + 1]) & 0x3F) << 6) |
					(static_cast<unsigned char>(str[i + 2]) & 0x3F);
				i += 3;
			}
			else if ((c >> 3) == 0x1E) // 4 bytes
			{
				if (i + 3 >= str.size())
					break;

				codepoint = ((c & 0x07) << 18) | ((static_cast<unsigned char>(str[i + 1]) & 0x3F) << 12) |
					((static_cast<unsigned char>(str[i + 2]) & 0x3F) << 6) |
					(static_cast<unsigned char>(str[i + 3]) & 0x3F);
				i += 4;
			}
			else
			{
				// invalid byte → skip or replace
				++i;
				continue;
			}

			result.push_back(static_cast<char32_t>(codepoint));
		}

		return result;
	}

	inline void from_json(const nlohmann::json& j, FontProvider& fp)
	{
		j.at("file").get_to(fp.file);

		j.at("ascent").get_to(fp.ascent);

		std::vector<std::string> utf8Rows;
		j.at("chars").get_to(utf8Rows);

		fp.chars.clear();

		for (int row = 0; row < static_cast<int>(utf8Rows.size()); ++row)
		{
			std::u32string row32 = Utf8ToUtf32(utf8Rows[row]);

			for (int col = 0; col < static_cast<int>(row32.size()); ++col)
			{
				char32_t codepoint = row32[col];

				// Skip null entries
				if (codepoint == U'\0')
					continue;

				fp.chars[codepoint] = {row, col};
			}

			fp.totalCols = std::max(fp.totalCols, static_cast<int>(row32.size()));
			fp.totalRows = std::max(fp.totalRows, row + 1);
		}
	}

	struct FontProviders
	{
		std::vector<FontProvider> providers;
	};

	inline void from_json(const nlohmann::json& j, FontProviders& fp)
	{
		j.at("providers").get_to(fp.providers);
	}
} // namespace onion::voxel
