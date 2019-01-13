#pragma once

#include "MapStore.h"

namespace rtdoom
{
#pragma pack(1)
	struct Palette
	{
		struct Color24
		{
			uint8_t r;
			uint8_t g;
			uint8_t b;
		};

		Color24 colors[256];
	};

	struct Texture
	{
		std::string name;
		int masked;
		short width;
		short height;
		std::unique_ptr<unsigned char[]> pixels;
	};
#pragma pack()

	class WADFile
	{
	protected:

#pragma pack(1)
		struct Header
		{
			char wadType[4];
			int numEntries;
			int dirLocation;
		};

		struct Lump
		{
			int dataOffset;
			int dataSize;
			char lumpName[8];
		};

		struct Patch
		{
			short width;
			short height;
			short left;
			short top;
			std::unique_ptr<unsigned char[]> pixels;
		};

		struct PatchInfo
		{
			short originx;
			short originy;
			short patch;
			short stepdir;
			short colormap;
		};

		struct TextureInfo
		{
			char name[8];
			int masked;
			short width;
			short height;
			int directory;
			short patchcount;
		};
#pragma pack()

		std::vector<char> LoadLump(std::ifstream& infile, const Lump& lump);

		enum class LumpType { Unknown, MapMarker, Palette, Texture, PatchNames, PatchesStart, PatchesEnd, FlatsStart, FlatsEnd, SpritesStart, SpritesEnd };

		LumpType GetLumpType(const Lump& lumpName) const;

		std::map<std::string, std::shared_ptr<Patch>> m_patches;

		std::vector<std::string> m_patchNames;

		void PastePatch(Texture * texture, const Patch * patch, int x, int y);

		std::shared_ptr<Patch> LoadPatch(const std::vector<char>& patchLump);

	public:
		Palette m_palette;

		static const std::map<int, std::string> m_thingTypes;
		std::map<std::string, MapStore> m_maps;
		std::map<std::string, std::shared_ptr<Texture>> m_textures;
		std::map<std::string, std::shared_ptr<Patch>> m_sprites;

		WADFile(const std::string& fileName);
		~WADFile();
	};
}
