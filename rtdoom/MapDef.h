#pragma once

#include "MapStore.h"
#include "MapStructs.h"

namespace rtdoom
{
class MapDef
{
protected:
    MapStore m_store;

    static bool IsInFrontOf(const Point& pov, const MapStore::Node& node) noexcept;
    static bool IsInFrontOf(const Point& pov, const Vertex& sv, const Vertex& ev) noexcept;

    void LookupVertex(unsigned short vertexNo, float& x, float& y);
    void ProcessSegment(float sx, float sy, float ex, float ey, unsigned short lineDefNo, signed short direction, signed short offset);
    void ProcessNode(const Point& pov, const MapStore::Node& node, std::deque<std::shared_ptr<SubSector>>& subSectors) const;
    void ProcessChildRef(unsigned short childRef, const Point& pov, std::deque<std::shared_ptr<SubSector>>& subSectors) const;
    void ProcessSubsector(std::shared_ptr<SubSector> subSector, std::deque<std::shared_ptr<SubSector>>& subSectors) const;
    void OpenDoors();
    void Initialize();
    void BuildWireframe();
    void BuildSegments();
    void BuildSectors();
    void BuildSubSectors();
    void BuildThings();

public:
    static bool                             IsInFrontOf(const Point& pov, const Line& line) noexcept;
    bool                                    HasGL() const;
    std::vector<Line>                       m_wireframe;
    std::vector<Sector>                     m_sectors;
    std::vector<std::vector<Thing>>         m_things;
    std::vector<std::shared_ptr<Segment>>   m_segments;
    std::vector<std::shared_ptr<SubSector>> m_subSectors;

    Thing                                  GetStartingPosition() const;
    std::optional<Sector>                  GetSector(const Point& pov) const;
    std::deque<std::shared_ptr<Segment>>   GetSegmentsToDraw(const Point& pov) const;
    std::deque<std::shared_ptr<SubSector>> GetSubSectorsToDraw(const Point& pov) const;

    MapDef(const std::string& mapFolder);
    MapDef(const MapStore& mapStore);
    ~MapDef();
};
} // namespace rtdoom
