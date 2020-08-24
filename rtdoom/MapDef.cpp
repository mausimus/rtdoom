#include "pch.h"
#include "rtdoom.h"
#include "MapDef.h"
#include "WADFile.h"

using std::vector;
using std::deque;
using std::string;
using std::shared_ptr;

namespace rtdoom
{
MapDef::MapDef(const MapStore& mapStore) : m_store {mapStore}
{
    Initialize();
}

MapDef::MapDef(const string& mapFolder)
{
    m_store.Load(mapFolder);
    Initialize();
}

void MapDef::Initialize()
{
    OpenDoors();
    BuildWireframe();
    BuildSegments();
    BuildSectors();
    BuildSubSectors();
    BuildThings();
}

void MapDef::OpenDoors()
{
    // open all doors on the map
    std::set<int> doorSectors;
    for(const auto& lineDef : m_store.m_lineDefs)
    {
        if((lineDef.lineType == 1 || lineDef.lineType == 31) && lineDef.leftSideDef < 32000) // TODO: all door linedefs
        {
            const auto& backSide = m_store.m_sideDefs[lineDef.leftSideDef];
            doorSectors.insert(backSide.sector);
        }
    }
    for(auto doorSector : doorSectors)
    {
        m_store.m_sectors[doorSector].ceilingHeight =
            m_store.m_sectors[doorSector].floorHeight + 72; // TODO: 4 less than the lowest neighbouring sector
    }
}

// traverse the BSP tree stored with the map depth-first
deque<shared_ptr<Segment>> MapDef::GetSegmentsToDraw(const Point& pov) const
{
    deque<shared_ptr<Segment>> segments;
    const auto&                subSectors = GetSubSectorsToDraw(pov);
    for(auto& subSector : subSectors)
    {
        for(auto& segment : subSector->segments)
        {
            segments.push_back(segment);
        }
    }
    return segments;
}

deque<shared_ptr<SubSector>> MapDef::GetSubSectorsToDraw(const Point& pov) const
{
    deque<shared_ptr<SubSector>> subSectors;
    ProcessNode(pov, *m_store.m_nodes.rbegin(), subSectors);
    return subSectors;
}

// find the sector where this point is located
std::optional<Sector> MapDef::GetSector(const Point& pov) const
{
    auto treeIndex = m_store.m_nodes.size() - 1;
    while(true)
    {
        const auto&    treeNode = m_store.m_nodes[treeIndex];
        unsigned short childRef = IsInFrontOf(pov, treeNode) ? treeNode.rightChild : treeNode.leftChild;
        if(childRef & 0x8000)
        {
            const SubSector& subSector = *m_subSectors.at(childRef & 0x7fff);
//            for(auto i = 0; i < m_store.m_subSectors[childRef & 0x7fff].numSegments; i++)
            for(auto segment : subSector.segments)
            {
//                const auto& ld = m_store.m_lineDefs[m_store.m_segments[m_store.m_subSectors[childRef & 0x7fff].firstSegment + i].lineDef];
 //               const bool  isInFront = IsInFrontOf(pov, m_store.m_vertexes[ld.startVertex], m_store.m_vertexes[ld.endVertex]);
                const bool  isInFront = IsInFrontOf(pov, segment->s, segment->e);

//                if(ld.leftSideDef < 32000 && !isInFront)
                if(!segment->frontSide.sideless && !isInFront)
                {
                    const auto sectorId = subSector.sectorId;//                    m_store.m_sideDefs[ld.leftSideDef].sector;
                    return m_sectors[sectorId];
                }
  //              else if(ld.rightSideDef < 32000 && isInFront)
                else if(!segment->backSide.sideless && isInFront)
                {
                    const auto sectorId = subSector.sectorId;//                    m_store.m_sideDefs[ld.rightSideDef].sector;
                    return m_sectors[sectorId];
                }
            }
            return std::nullopt;
        }
        else
        {
            treeIndex = childRef;
        }
    }
}

// process tree node and go left or right depending on the locationo of player vs the BSP division line
void MapDef::ProcessNode(const Point& pov, const MapStore::Node& node, deque<shared_ptr<SubSector>>& subSectors) const
{
    if(IsInFrontOf(pov, node))
    {
        ProcessChildRef(node.rightChild, pov, subSectors);
        ProcessChildRef(node.leftChild, pov, subSectors);
    }
    else
    {
        ProcessChildRef(node.leftChild, pov, subSectors);
        ProcessChildRef(node.rightChild, pov, subSectors);
    }
}

// process a child node depending on whether it's an inner node or a leaf (subsector)
void MapDef::ProcessChildRef(unsigned short childRef, const Point& pov, deque<shared_ptr<SubSector>>& subSectors) const
{
    if(childRef & 0x8000)
    {
        // ref is a subsector
        ProcessSubsector(m_subSectors[childRef & 0x7fff], subSectors);
    }
    else
    {
        // ref is another node
        ProcessNode(pov, m_store.m_nodes[childRef], subSectors);
    }
}

Thing MapDef::GetStartingPosition() const
{
    signed short   px, py;
    unsigned short pa;
    m_store.GetStartingPosition(px, py, pa);

    Thing player(static_cast<float>(px), static_cast<float>(py), 0, static_cast<float>(pa / 180.0f * PI));

    return player;
}

bool MapDef::IsInFrontOf(const Point& pov, const Line& line) noexcept
{
    const auto deltaX = line.e.x - line.s.x;
    const auto deltaY = line.e.y - line.s.y;

    if(deltaX == 0)
    {
        if(pov.x <= line.s.x)
        {
            return deltaY < 0;
        }
        return deltaY > 0;
    }
    if(deltaY == 0)
    {
        if(pov.y <= line.s.y)
        {
            return deltaX > 0;
        }
        return deltaX < 0;
    }

    // use cross-product to determine which side the point is on
    const auto dx    = pov.x - line.s.x;
    const auto dy    = pov.y - line.s.y;
    const auto left  = deltaY * dx;
    const auto right = dy * deltaX;
    return right < left;
}

bool MapDef::IsInFrontOf(const Point& pov, const MapStore::Node& node) noexcept
{
    return IsInFrontOf(
        pov, Line(Vertex {node.partitionX, node.partitionY}, Vertex {node.partitionX + node.deltaX, node.partitionY + node.deltaY}));
}

bool MapDef::IsInFrontOf(const Point& pov, const Vertex& sv, const Vertex& ev) noexcept
{
    return IsInFrontOf(pov, Line(sv, ev));
}

void MapDef::BuildWireframe()
{
    for(const auto& l : m_store.m_lineDefs)
    {
        const auto& sv = m_store.m_vertexes[l.startVertex];
        const auto& ev = m_store.m_vertexes[l.endVertex];
        m_wireframe.push_back(Line(Vertex(sv.x, sv.y), Vertex(ev.x, ev.y)));
    }
}

void MapDef::ProcessSegment(float sx, float sy, float ex, float ey, unsigned short lineDefNo, signed short direction, signed short offset)
{
    const Vertex s {sx, sy};
    const Vertex e {ex, ey};

    if(lineDefNo != 65535)
    {
        const auto&       lineDef = m_store.m_lineDefs[lineDefNo];
        bool              isSolid = lineDef.leftSideDef > 32000 || lineDef.rightSideDef > 32000;
        Sector            frontSector, backSector;
        MapStore::SideDef frontSide, backSide;
        if(direction == 1 && lineDef.leftSideDef < 32000)
        {
            frontSide   = m_store.m_sideDefs[lineDef.leftSideDef];
            frontSector = Sector {frontSide.sector, m_store.m_sectors[frontSide.sector]};
            if(lineDef.rightSideDef < 32000)
            {
                backSide   = m_store.m_sideDefs[lineDef.rightSideDef];
                backSector = Sector {backSide.sector, m_store.m_sectors[backSide.sector]};
            }
        }
        else if(direction == 0 && lineDef.rightSideDef < 32000)
        {
            frontSide   = m_store.m_sideDefs[lineDef.rightSideDef];
            frontSector = Sector {frontSide.sector, m_store.m_sectors[frontSide.sector]};
            if(lineDef.leftSideDef < 32000)
            {
                backSide   = m_store.m_sideDefs[lineDef.leftSideDef];
                backSector = Sector {backSide.sector, m_store.m_sectors[backSide.sector]};
            }
        }

        Side front {frontSector,
                    Helpers::MakeString(frontSide.lowerTexture),
                    Helpers::MakeString(frontSide.middleTexture),
                    Helpers::MakeString(frontSide.upperTexture),
                    frontSide.xOffset,
                    frontSide.yOffset};
        Side back {backSector,
                   Helpers::MakeString(backSide.lowerTexture),
                   Helpers::MakeString(backSide.middleTexture),
                   Helpers::MakeString(backSide.upperTexture),
                   frontSide.xOffset,
                   frontSide.yOffset};

        m_segments.push_back(
            std::make_shared<Segment>(s, e, isSolid, front, back, offset, (bool)(lineDef.flags & 0x0010), (bool)(lineDef.flags & 0x0008)));
    }
    else
    {
        m_segments.push_back(std::make_shared<Segment>(s, e, false, Side(), Side(), offset, false, false));
    }
}

void MapDef::LookupVertex(unsigned short vertexNo, float& x, float& y)
{
    if(vertexNo & (1 << 15))
    {
        const auto& vertex = m_store.m_glVertexes[vertexNo ^ (1 << 15)];
        x                  = vertex.x;
        y                  = vertex.y;
    }
    else
    {
        const auto& vertex = m_store.m_vertexes[vertexNo];
        x                  = vertex.x;
        y                  = vertex.y;
    }
}

void MapDef::BuildSegments()
{
    if(m_store.m_glSegments.size())
    {
        m_segments.reserve(m_store.m_glSegments.size());
        for(const auto& mapSegment : m_store.m_glSegments)
        {
            float sx, sy, ex, ey;
            LookupVertex(mapSegment.startVertex, sx, sy);
            LookupVertex(mapSegment.endVertex, ex, ey);
            ProcessSegment(sx,
                           sy,
                           ex,
                           ey,
                           mapSegment.lineDef,
                           mapSegment.side,
                           0 /*mapSegment.offset*/);
        }
    }
    else
    {
        m_segments.reserve(m_store.m_segments.size());
        for(const auto& mapSegment : m_store.m_segments)
        {
            const auto& mapStartVertex = m_store.m_vertexes[mapSegment.startVertex];
            const auto& mapEndVertex   = m_store.m_vertexes[mapSegment.endVertex];
            ProcessSegment(mapStartVertex.x,
                           mapStartVertex.y,
                           mapEndVertex.x,
                           mapEndVertex.y,
                           mapSegment.lineDef,
                           mapSegment.direction,
                           mapSegment.offset);
        }
    }
}

// process serialized map format into usable structures
void MapDef::ProcessSubsector(std::shared_ptr<SubSector> subSector, deque<std::shared_ptr<SubSector>>& subSectors) const
{
    subSectors.push_back(subSector);
}

void MapDef::BuildSectors()
{
    int s = 0;
    for(const auto& sector : m_store.m_sectors)
    {
        m_sectors.emplace_back(Sector(s++, sector));
    }
}

bool MapDef::HasGL() const {
    return m_store.m_glSegments.size();
}

void MapDef::BuildSubSectors()
{
    int subSectorId = 0;
    for(const auto& subSector : m_store.m_subSectors)
    {
        int                         sectorId = -1;
        vector<shared_ptr<Segment>> segments;
        for(int i = 0; i < subSector.numSegments; i++)
        {
            shared_ptr<Segment> segment = m_segments[subSector.firstSegment + i];
            if(!segment->frontSide.sideless)
            {
                sectorId = segment->frontSide.sector.sectorId;
            }
            segments.push_back(segment);
        }
        m_subSectors.push_back(std::make_shared<SubSector>(subSectorId++, sectorId, segments));
    }
}

void MapDef::BuildThings()
{
    int id = 0;
    m_things.resize(m_sectors.size());
    for(const auto& thing : m_store.m_things)
    {
        Thing      t(static_cast<float>(thing.x), static_cast<float>(thing.y), 0, static_cast<float>(thing.a / 180.0f * PI));
        const auto sector = GetSector(t);
        if(sector.has_value())
        {
            t.sectorId     = sector->sectorId;
            t.thingId      = id++;
            t.type         = thing.type;
            const auto& tt = WADFile::m_thingTypes.find(thing.type);
            if(tt != WADFile::m_thingTypes.end())
            {
                t.textureName = tt->second;
            }
            t.z = m_store.m_sectors[t.sectorId].floorHeight;
            m_things[t.sectorId].emplace_back(t);
        }
    }
}

MapDef::~MapDef() { }
} // namespace rtdoom
