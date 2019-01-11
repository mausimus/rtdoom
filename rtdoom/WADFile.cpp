#include "pch.h"
#include "WADFile.h"
#include "Helpers.h"

namespace rtdoom
{
	WADFile::WADFile(const std::string& fileName)
	{
		std::ifstream infile(fileName, std::ios::binary);
		if (!infile.good())
		{
			throw std::runtime_error("Unable to open file " + fileName);
		}

		Header header;
		infile.read(reinterpret_cast<char *>(&header), sizeof(header));
		infile.clear();
		infile.seekg(header.dirLocation);

		std::vector<Lump> lumps;
		for (auto i = 0; i < header.numEntries; i++)
		{
			Lump lump;
			infile.read(reinterpret_cast<char *>(&lump), sizeof(lump));
			lumps.push_back(lump);
		}

		// find and load maps and patches
		for (size_t i = 0; i < lumps.size(); i++)
		{
			const Lump& lump = lumps.at(i);
			switch (GetLumpType(lump))
			{
			case LumpType::MapMarker:
			{
				std::map<std::string, std::vector<char>> mapLumps;
				for (auto j = 1; j <= 10; j++)
				{
					std::string lumpName = Helpers::MakeString(lumps[i + j].lumpName);
					mapLumps.insert(make_pair(lumpName, LoadLump(infile, lumps[i + j])));
				}
				i += 10;
				MapStore mapStore;
				mapStore.Load(mapLumps);
				m_maps.insert(make_pair(std::string(lump.lumpName), mapStore));
				break;
			}
			case LumpType::Palette:
			{
				Helpers::LoadEntity<Palette>(LoadLump(infile, lump), &m_palette);
				break;
			}
			case LumpType::PatchNames:
			{
				auto lumpData = LoadLump(infile, lump);
				int numPatches;
				memcpy(&numPatches, lumpData.data(), sizeof(numPatches));
				for (int pi = 0; pi < numPatches; pi++)
				{
					char patchName[8];
					memcpy(patchName, lumpData.data() + sizeof(numPatches) + pi * sizeof(char[8]), sizeof(char[8]));
					m_patchNames.push_back(Helpers::MakeString(patchName));
				}
				break;
			}
			case LumpType::PatchesStart:
			{
				int j = 1;
				while (GetLumpType(lumps.at(i + j)) != LumpType::PatchesEnd)
				{
					const Lump& patchLump = lumps.at(i + j);
					auto patchData = LoadLump(infile, patchLump);
					auto p = LoadPatch(patchData);
					m_patches.insert(make_pair(Helpers::MakeString(patchLump.lumpName), p));
					j++;
				}
				break;
			}
			case LumpType::SpritesStart:
			{
				int j = 1;
				while (GetLumpType(lumps.at(i + j)) != LumpType::SpritesEnd)
				{
					const Lump& patchLump = lumps.at(i + j);
					auto patchData = LoadLump(infile, patchLump);
					auto p = LoadPatch(patchData);
					m_sprites.insert(make_pair(Helpers::MakeString(patchLump.lumpName), p));
					j++;
				}
				break;
			}
			case LumpType::FlatsStart:
			{
				int j = 1;
				while (GetLumpType(lumps.at(i + j)) != LumpType::FlatsEnd)
				{
					const Lump& patchLump = lumps.at(i + j);
					auto patchData = LoadLump(infile, patchLump);

					auto t = std::make_shared<Texture>();
					t->width = 64;
					t->height = 64;
					t->name = Helpers::MakeString(patchLump.lumpName);
					t->masked = 0;
					t->pixels = std::make_unique<unsigned char[]>(t->width * t->height);

					memcpy(t->pixels.get(), patchData.data(), 64 * 64);

					m_textures.insert(make_pair(t->name, t));
					j++;
				}
				break;
			}
			default:
				break;
			}
		}

		// build textures
		for (const auto& lump : lumps)
		{
			switch (GetLumpType(lump))
			{
			case LumpType::Texture:
			{
				auto data = LoadLump(infile, lump);
				signed int numTextures;
				memcpy(&numTextures, data.data(), sizeof(numTextures));
				std::vector<int> textureOffsets;
				for (auto i = 0; i < numTextures; i++)
				{
					int textureOffset;
					memcpy(&textureOffset, data.data() + sizeof(numTextures) + i * sizeof(textureOffset), sizeof(textureOffset));

					TextureInfo textureInfo;
					memcpy(&textureInfo, data.data() + textureOffset, sizeof(TextureInfo));
					std::vector<PatchInfo> patches;
					for (auto j = 0; j < textureInfo.patchcount; j++)
					{
						PatchInfo p;
						memcpy(&p, data.data() + textureOffset + sizeof(TextureInfo) + j * sizeof(PatchInfo), sizeof(PatchInfo));
						patches.push_back(p);
					}

					auto t = std::make_shared<Texture>();
					t->width = textureInfo.width;
					t->height = textureInfo.height;
					t->name = Helpers::MakeString(textureInfo.name);
					t->masked = textureInfo.masked;
					t->pixels = std::make_unique<unsigned char[]>(t->width * t->height);

					for (const auto& p : patches)
					{
						const auto pi = m_patchNames[p.patch];
						PastePatch(t.get(), m_patches[pi].get(), p.originx, p.originy);
					}

					m_textures.insert(make_pair(t->name, t));
				}
			}
			break;
			}
		}
	}

	std::shared_ptr<WADFile::Patch> WADFile::LoadPatch(const std::vector<char>& patchData)
	{
		auto p = std::make_shared<Patch>();
		memcpy(p.get(), patchData.data(), 4 * sizeof(short));
		p->pixels = std::make_unique<unsigned char[]>(p->width * p->height);
		memset(p->pixels.get(), static_cast<unsigned char>(247), p->width * p->height);

		std::vector<int> columnArray(p->width);
		memcpy(columnArray.data(), patchData.data() + 4 * sizeof(short), sizeof(int) * p->width);

		for (auto c = 0; c < p->width; c++)
		{
			auto columnOffset = columnArray[c];
			unsigned char rowstart = 0;
			while (rowstart != 255)
			{
				memcpy(&rowstart, patchData.data() + columnOffset++, sizeof(unsigned char));
				if (rowstart == 255)
				{
					break;
				}
				unsigned char pixelCount;
				unsigned char dummy;
				memcpy(&pixelCount, patchData.data() + columnOffset++, sizeof(unsigned char));
				memcpy(&dummy, patchData.data() + columnOffset++, sizeof(unsigned char));
				for (unsigned char pj = 0; pj < pixelCount; pj++)
				{
					unsigned char pixelColor;
					memcpy(&pixelColor, patchData.data() + columnOffset++, sizeof(unsigned char));
					p->pixels[(pj + rowstart) * p->width + c] = pixelColor;
				}
				memcpy(&dummy, patchData.data() + columnOffset++, sizeof(unsigned char));
			}
		}
		return p;
	}

	WADFile::LumpType WADFile::GetLumpType(const Lump& lump) const
	{
		if (lump.dataSize == 0)
		{
			if ((lump.lumpName[0] == 'E' && lump.lumpName[2] == 'M') || (lump.lumpName[0] == 'M' && lump.lumpName[1] == 'A' && lump.lumpName[2] == 'P'))
			{
				return LumpType::MapMarker;
			}
			if (lump.lumpName[0] == 'P' && isdigit(lump.lumpName[1]) && lump.lumpName[2] == '_')
			{
				if (lump.lumpName[3] == 'S')
				{
					return LumpType::PatchesStart;
				}
				if (lump.lumpName[3] == 'E')
				{
					return LumpType::PatchesEnd;
				}
			}
			if (lump.lumpName[0] == 'F' && isdigit(lump.lumpName[1]) && lump.lumpName[2] == '_')
			{
				if (lump.lumpName[3] == 'S')
				{
					return LumpType::FlatsStart;
				}
				if (lump.lumpName[3] == 'E')
				{
					return LumpType::FlatsEnd;
				}
			}
			if (lump.lumpName[0] == 'S' && lump.lumpName[1] == '_')
			{
				if (lump.lumpName[2] == 'S')
				{
					return LumpType::SpritesStart;
				}
				if (lump.lumpName[2] == 'E')
				{
					return LumpType::SpritesEnd;
				}
			}
			return LumpType::Unknown;
		}
		if (strncmp(lump.lumpName, "PLAYPAL", 7) == 0)
		{
			return LumpType::Palette;
		}
		if (strncmp(lump.lumpName, "TEXTURE", 7) == 0)
		{
			return LumpType::Texture;
		}
		if (strncmp(lump.lumpName, "PNAMES", 7) == 0)
		{
			return LumpType::PatchNames;
		}
		return LumpType::Unknown;
	}

	void WADFile::PastePatch(Texture * texture, const Patch * patch, int px, int py)
	{
		for (auto x = 0; x < patch->width; x++)
		{
			for (auto y = 0; y < patch->height; y++)
			{
				const auto colorIndex = patch->pixels[y * patch->width + x];
				if (colorIndex != 247)
				{
					const auto color = m_palette.colors[colorIndex];
					auto dx = px + x;
					auto dy = py + y;
					if (dx < 0 || dx >= texture->width || dy < 0 || dy >= texture->height)
					{
						continue;
					}

					texture->pixels[dy * texture->width + dx] = patch->pixels[y * patch->width + x];
				}
			}
		}
	}

	std::vector<char> WADFile::LoadLump(std::ifstream& infile, const Lump& lump)
	{
		std::vector<char> data;
		infile.clear();
		infile.seekg(lump.dataOffset);
		data.resize(lump.dataSize);
		infile.read(data.data(), lump.dataSize);
		return data;
	}

	WADFile::~WADFile()
	{
	}
}