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
    static bool IsInFrontOf(const Point& pov, const MapStore::Vertex& sv, const MapStore::Vertex& ev) noexcept;

    void ProcessNode(const Point& pov, const MapStore::Node& node, std::deque<std::shared_ptr<Segment>>& segments) const;
    void ProcessChildRef(unsigned short childRef, const Point& pov, std::deque<std::shared_ptr<Segment>>& segments) const;
    void ProcessSubsector(const MapStore::SubSector& subSector, std::deque<std::shared_ptr<Segment>>& segments) const;
    void OpenDoors();
    void Initialize();
    void BuildWireframe();
    void BuildSegments();
    void BuildSectors();
    void BuildThings();

public:
    static bool                           IsInFrontOf(const Point& pov, const Line& line) noexcept;
    std::vector<Line>                     m_wireframe;
    std::vector<Sector>                   m_sectors;
    std::vector<std::vector<Thing>>       m_things;
    std::vector<std::shared_ptr<Segment>> m_segments;

    Thing                                GetStartingPosition() const;
    std::optional<Sector>                GetSector(const Point& pov) const;
    std::deque<std::shared_ptr<Segment>> GetSegmentsToDraw(const Point& pov) const;

    MapDef(const std::string& mapFolder);
    MapDef(const MapStore& mapStore);
    ~MapDef();
};
} // namespace rtdoom
