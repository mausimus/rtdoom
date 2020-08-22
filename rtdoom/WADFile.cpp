#include "pch.h"
#include "WADFile.h"
#include "Helpers.h"

namespace rtdoom
{
WADFile::WADFile(const std::string& fileName)
{
    std::ifstream infile(fileName, std::ios::binary);
    if(!infile.good())
    {
        throw std::runtime_error("Unable to open file " + fileName);
    }

    Header header;
    infile.read(reinterpret_cast<char*>(&header), sizeof(header));
    infile.clear();
    infile.seekg(header.dirLocation);

    std::vector<Lump> lumps;
    for(auto i = 0; i < header.numEntries; i++)
    {
        Lump lump;
        infile.read(reinterpret_cast<char*>(&lump), sizeof(lump));
        lumps.push_back(lump);
    }

    // find and load maps and patches
    for(size_t i = 0; i < lumps.size(); i++)
    {
        const Lump& lump = lumps.at(i);
        switch(GetLumpType(lump))
        {
        case LumpType::MapMarker: {
            std::map<std::string, std::vector<char>> mapLumps;
            for(auto j = 1; j <= 10; j++)
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
        case LumpType::Palette: {
            Helpers::LoadEntity<Palette>(LoadLump(infile, lump), &m_palette);
            break;
        }
        case LumpType::PatchNames: {
            auto lumpData = LoadLump(infile, lump);
            int  numPatches;
            memcpy(&numPatches, lumpData.data(), sizeof(numPatches));
            for(int pi = 0; pi < numPatches; pi++)
            {
                char patchName[8];
                memcpy(patchName, lumpData.data() + sizeof(numPatches) + pi * sizeof(char[8]), sizeof(char[8]));
                m_patchNames.push_back(Helpers::MakeString(patchName));
            }
            break;
        }
        case LumpType::PatchesStart: {
            int j = 1;
            while(GetLumpType(lumps.at(i + j)) != LumpType::PatchesEnd)
            {
                const Lump& patchLump = lumps.at(i + j);
                auto        patchData = LoadLump(infile, patchLump);
                auto        p         = LoadPatch(patchData);
                m_patches.insert(make_pair(Helpers::MakeString(patchLump.lumpName), p));
                j++;
            }
            break;
        }
        case LumpType::SpritesStart: {
            int j = 1;
            while(GetLumpType(lumps.at(i + j)) != LumpType::SpritesEnd)
            {
                const Lump& patchLump = lumps.at(i + j);
                auto        patchData = LoadLump(infile, patchLump);
                auto        p         = LoadPatch(patchData);
                const auto& patchName = Helpers::MakeString(patchLump.lumpName);
                if(patchName.length() == 8 && isdigit(patchName[5]) && isdigit(patchName[7]))
                {
                    // split into two patches
                    m_sprites.insert(make_pair(patchName.substr(0, 6), p));
                    m_sprites.insert(make_pair(patchName.substr(0, 4) + patchName.substr(6, 2), FlipPatch(p)));
                }
                else
                {
                    m_sprites.insert(make_pair(patchName, p));
                }
                j++;
            }
            break;
        }
        case LumpType::FlatsStart: {
            int j = 1;
            while(GetLumpType(lumps.at(i + j)) != LumpType::FlatsEnd)
            {
                const Lump& patchLump = lumps.at(i + j);
                auto        patchData = LoadLump(infile, patchLump);

                auto t    = std::make_shared<Texture>();
                t->width  = 64;
                t->height = 64;
                t->name   = Helpers::MakeString(patchLump.lumpName);
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
    for(const auto& lump : lumps)
    {
        switch(GetLumpType(lump))
        {
        case LumpType::Texture: {
            auto       data = LoadLump(infile, lump);
            signed int numTextures;
            memcpy(&numTextures, data.data(), sizeof(numTextures));
            std::vector<int> textureOffsets;
            for(auto i = 0; i < numTextures; i++)
            {
                int textureOffset;
                memcpy(&textureOffset, data.data() + sizeof(numTextures) + i * sizeof(textureOffset), sizeof(textureOffset));

                TextureInfo textureInfo;
                memcpy(&textureInfo, data.data() + textureOffset, sizeof(TextureInfo));
                std::vector<PatchInfo> patches;
                for(auto j = 0; j < textureInfo.patchcount; j++)
                {
                    PatchInfo p;
                    memcpy(&p, data.data() + textureOffset + sizeof(TextureInfo) + j * sizeof(PatchInfo), sizeof(PatchInfo));
                    patches.push_back(p);
                }

                auto t    = std::make_shared<Texture>();
                t->width  = textureInfo.width;
                t->height = textureInfo.height;
                t->name   = Helpers::MakeString(textureInfo.name);
                t->masked = textureInfo.masked;
                t->pixels = std::make_unique<unsigned char[]>(t->width * t->height);
                memset(t->pixels.get(), static_cast<unsigned char>(247), t->width * t->height);

                for(const auto& p : patches)
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

    TryLoadGWA(fileName);
}

void WADFile::TryLoadGWA(const std::string& fileName)
{
    std::string glName(fileName);
    glName.replace(glName.find_last_of('.'), glName.length(), ".gwa");

    std::ifstream infile(glName, std::ios::binary);
    if(!infile.good())
    {
        return;
    }

    Header header;
    infile.read(reinterpret_cast<char*>(&header), sizeof(header));
    infile.clear();
    infile.seekg(header.dirLocation);

    std::vector<Lump> lumps;
    for(auto i = 0; i < header.numEntries; i++)
    {
        Lump lump;
        infile.read(reinterpret_cast<char*>(&lump), sizeof(lump));
        lumps.push_back(lump);
    }

    // find and load maps and patches
    for(size_t i = 0; i < lumps.size(); i++)
    {
        const Lump& lump = lumps.at(i);
        switch(GetLumpType(lump))
        {
        case LumpType::MapMarker: {
            std::map<std::string, std::vector<char>> mapLumps;
            for(auto j = 1; j <= 4; j++)
            {
                std::string lumpName = Helpers::MakeString(lumps[i + j].lumpName);
                mapLumps.insert(make_pair(lumpName, LoadLump(infile, lumps[i + j])));
            }
            i += 4;
            const std::string mapName(lump.lumpName + 3);
            auto map = m_maps.find(mapName);
            if(map != m_maps.end())
            {
                map->second.LoadGL(mapLumps);
            }
            break;
        }
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

    for(auto c = 0; c < p->width; c++)
    {
        auto          columnOffset = columnArray[c];
        unsigned char rowstart     = 0;
        while(rowstart != 255)
        {
            memcpy(&rowstart, patchData.data() + columnOffset++, sizeof(unsigned char));
            if(rowstart == 255)
            {
                break;
            }
            unsigned char pixelCount;
            unsigned char dummy;
            memcpy(&pixelCount, patchData.data() + columnOffset++, sizeof(unsigned char));
            memcpy(&dummy, patchData.data() + columnOffset++, sizeof(unsigned char));
            for(unsigned char pj = 0; pj < pixelCount; pj++)
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

std::shared_ptr<WADFile::Patch> WADFile::FlipPatch(const std::shared_ptr<WADFile::Patch>& patch)
{
    auto p    = std::make_shared<Patch>();
    p->height = patch->height;
    p->width  = patch->width;
    p->left   = patch->width - patch->left - 1;
    p->top    = patch->top;
    p->pixels = std::make_unique<unsigned char[]>(p->width * p->height);
    for(int y = 0; y < patch->height; y++)
    {
        for(int x = 0; x < patch->width; x++)
        {
            p->pixels[p->width * y + x] = patch->pixels[patch->width * y + patch->width - 1 - x];
        }
    }
    return p;
}

WADFile::LumpType WADFile::GetLumpType(const Lump& lump) const
{
    if(lump.dataSize == 0)
    {
        if((lump.lumpName[0] == 'E' && lump.lumpName[2] == 'M') ||
           (lump.lumpName[0] == 'M' && lump.lumpName[1] == 'A' && lump.lumpName[2] == 'P'))
        {
            return LumpType::MapMarker;
        }
        if(lump.lumpName[0] == 'P' && isdigit(lump.lumpName[1]) && lump.lumpName[2] == '_')
        {
            if(lump.lumpName[3] == 'S')
            {
                return LumpType::PatchesStart;
            }
            if(lump.lumpName[3] == 'E')
            {
                return LumpType::PatchesEnd;
            }
        }
        if(lump.lumpName[0] == 'F' && isdigit(lump.lumpName[1]) && lump.lumpName[2] == '_')
        {
            if(lump.lumpName[3] == 'S')
            {
                return LumpType::FlatsStart;
            }
            if(lump.lumpName[3] == 'E')
            {
                return LumpType::FlatsEnd;
            }
        }
        if(lump.lumpName[0] == 'S' && lump.lumpName[1] == '_')
        {
            if(lump.lumpName[2] == 'S')
            {
                return LumpType::SpritesStart;
            }
            if(lump.lumpName[2] == 'E')
            {
                return LumpType::SpritesEnd;
            }
        }
        return LumpType::Unknown;
    }
    if(lump.lumpName[0] == 'G' && lump.lumpName[1] == 'L')
    {
        return LumpType::MapMarker;
    }
    if(strncmp(lump.lumpName, "PLAYPAL", 7) == 0)
    {
        return LumpType::Palette;
    }
    if(strncmp(lump.lumpName, "TEXTURE", 7) == 0)
    {
        return LumpType::Texture;
    }
    if(strncmp(lump.lumpName, "PNAMES", 7) == 0)
    {
        return LumpType::PatchNames;
    }
    return LumpType::Unknown;
}

void WADFile::PastePatch(Texture* texture, const Patch* patch, int px, int py)
{
    for(auto x = 0; x < patch->width; x++)
    {
        for(auto y = 0; y < patch->height; y++)
        {
            const auto colorIndex = patch->pixels[y * patch->width + x];
            if(colorIndex != 247)
            {
                const auto color = m_palette.colors[colorIndex];
                auto       dx    = px + x;
                auto       dy    = py + y;
                if(dx < 0 || dx >= texture->width || dy < 0 || dy >= texture->height)
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

WADFile::~WADFile() { }

const std::map<int, std::string> WADFile::m_thingTypes {
    {2023, "PSTRA0"}, {2026, "PMAPA0"}, {2014, "BON1A0"}, {2024, "PINSA0"}, {2022, "PINVA0"}, {2045, "PVISA0"}, {83, "MEGAA0"},
    {2013, "SOULA0"}, {2015, "BON2A0"}, {8, "BPAKA0"},    {2019, "ARM2A0"}, {2018, "ARM1A0"}, {2012, "MEDIA0"}, {2025, "SUITA0"},
    {2011, "STIMA0"}, {2006, "BFUGA0"}, {2002, "MGUNA0"}, {2005, "CSAWA0"}, {2004, "PLASA0"}, {2003, "LAUNA0"}, {2001, "SHOTA0"},
    {82, "SGN2A0"},   {2007, "CLIPA0"}, {2048, "AMMOA0"}, {2046, "BROKA0"}, {2049, "SBOXA0"}, {2047, "CELLA0"}, {17, "CELPA0"},
    {2010, "ROCKA0"}, {2008, "SHELA0"}, {5, "BKEYA0"},    {40, "BSKUA0"},   {13, "RKEYA0"},   {38, "RSKUA0"},   {6, "YKEYA0"},
    {39, "YSKUA0"},   {68, "BSPIA1"},   {64, "VILEA1"},   {3003, "BOSSA1"}, {3005, "HEADA1"}, {65, "CPOSA1"},   {72, "KEENA1"},
    {16, "CYBRA1"},   {3002, "SARGA1"}, {3004, "POSSA1"}, {9, "SPOSA1"},    {69, "BOS2A1"},   {3001, "TROOA1"}, {3006, "SKULA1"},
    {67, "FATTA1"},   {71, "PAINA1"},   {66, "SKELA1"},   {58, "SARGA1"},   {7, "SPIDA1"},    {84, "SSWVA1"},   {2035, "BAR1A0"},
    {70, "FCANA0"},   {43, "TRE1A0"},   {35, "CBRAA0"},   {41, "CEYEA0"},   {28, "POL2A0"},   {42, "FSKUA0"},   {2028, "COLUA0"},
    {53, "GOR5A0"},   {52, "GOR4A0"},   {78, "HDB6A0"},   {75, "HDB3A0"},   {77, "HDB5A0"},   {76, "HDB4A0"},   {50, "GOR2A0"},
    {74, "HDB2A0"},   {73, "HDB1A0"},   {51, "GOR3A0"},   {49, "GOR1A0"},   {25, "POL1A0"},   {54, "TRE2A0"},   {29, "POL3A0"},
    {55, "SMBTA0"},   {56, "SMGTA0"},   {31, "COL2A0"},   {36, "COL5A0"},   {57, "SMRTA0"},   {33, "COL4A0"},   {37, "COL6A0"},
    {86, "TLP2A0"},   {27, "POL4A0"},   {47, "SMITA0"},   {44, "TBLUA0"},   {45, "TGRNA0"},   {30, "COL1A0"},   {46, "TREDA0"},
    {32, "COL3A0"},   {85, "TLMPA0"},   {48, "ELECA0"},   {26, "POL6A0"},   {10, "PLAYW0"},   {12, "PLAYW0"},   {34, "CANDA0"},
    {22, "HEADL0"},   {21, "SARGN0"},   {18, "POSSL0"},   {19, "SPOSL0"},   {20, "TROOM0"},   {23, "SKULK0"},   {15, "PLAYN0"},
    {62, "GOR5A0"},   {60, "GOR4A0"},   {59, "GOR2A0"},   {61, "GOR3A0"},   {63, "GOR1A0"},   {79, "POB1A0"},   {80, "POB2A0"},
    {24, "POL5A0"},   {81, "BRS1A0"}};
} // namespace rtdoom
