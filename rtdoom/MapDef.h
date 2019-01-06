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

		void ProcessNode(const Point& pov, const MapStore::Node& node, std::deque<Segment>& segments) const;
		void ProcessChildRef(unsigned short childRef, const Point& pov, std::deque<Segment>& segments) const;
		void ProcessSubsector(const MapStore::SubSector& subSector, std::deque<Segment>& segments) const;
		void OpenDoors();
		void BuildWireframe();

	public:
		static bool IsInFrontOf(const Point& pov, const Line& line) noexcept;
		std::vector<Line> m_wireframe;

		Thing GetStartingPosition() const;
		std::deque<Segment> GetSegmentsToDraw(const Point& pov) const;

		MapDef(const std::string& mapFolder);
		MapDef(const MapStore& mapStore);
		~MapDef();
	};
}