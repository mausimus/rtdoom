#include "pch.h"
#include "MapStore.h"
#include "Helpers.h"

using namespace std::literals::string_literals;

namespace rtdoom
{
MapStore::MapStore() { }

void MapStore::Load(const std::string& mapFolder)
{
    m_vertexes   = Helpers::LoadEntities<Vertex>(mapFolder + "\\vertexes.lmp"s);
    m_lineDefs   = Helpers::LoadEntities<LineDef>(mapFolder + "\\linedefs.lmp"s);
    m_sideDefs   = Helpers::LoadEntities<SideDef>(mapFolder + "\\sidedefs.lmp"s);
    m_sectors    = Helpers::LoadEntities<Sector>(mapFolder + "\\sectors.lmp"s);
    m_subSectors = Helpers::LoadEntities<SubSector>(mapFolder + "\\ssectors.lmp"s);
    m_nodes      = Helpers::LoadEntities<Node>(mapFolder + "\\nodes.lmp"s);
    m_segments   = Helpers::LoadEntities<Segment>(mapFolder + "\\segs.lmp"s);
    m_things     = Helpers::LoadEntities<Thing>(mapFolder + "\\things.lmp"s);
}

void MapStore::Load(const std::map<std::string, std::vector<char>>& mapLumps)
{
    m_vertexes   = Helpers::LoadEntities<Vertex>(mapLumps.at("VERTEXES"s));
    m_lineDefs   = Helpers::LoadEntities<LineDef>(mapLumps.at("LINEDEFS"s));
    m_sideDefs   = Helpers::LoadEntities<SideDef>(mapLumps.at("SIDEDEFS"s));
    m_sectors    = Helpers::LoadEntities<Sector>(mapLumps.at("SECTORS"s));
    m_subSectors = Helpers::LoadEntities<SubSector>(mapLumps.at("SSECTORS"s));
    m_nodes      = Helpers::LoadEntities<Node>(mapLumps.at("NODES"s));
    m_segments   = Helpers::LoadEntities<Segment>(mapLumps.at("SEGS"s));
    m_things     = Helpers::LoadEntities<Thing>(mapLumps.at("THINGS"s));
}

void MapStore::LoadGL(const std::map<std::string, std::vector<char>>& glLumps)
{
    m_glVertexes = Helpers::LoadEntities<GLVertex>(glLumps.at("GL_VERT"s), 4);
    m_glSegments = Helpers::LoadEntities<GLSegment>(glLumps.at("GL_SEGS"s));
    m_subSectors = Helpers::LoadEntities<SubSector>(glLumps.at("GL_SSECT"s));
    m_nodes      = Helpers::LoadEntities<Node>(glLumps.at("GL_NODES"s));
}

void MapStore::GetStartingPosition(signed short& x, signed short& y, unsigned short& a) const
{
    auto foundPlayer = find_if(m_things.cbegin(), m_things.cend(), [](const Thing& t) { return t.type == 1; });
    if(foundPlayer == m_things.cend())
    {
        throw std::runtime_error("No player starting location on map");
    }
    const Thing& playerThing = *foundPlayer;
    x                        = playerThing.x;
    y                        = playerThing.y;
    a                        = playerThing.a;
}

MapStore::~MapStore() { }
} // namespace rtdoom
