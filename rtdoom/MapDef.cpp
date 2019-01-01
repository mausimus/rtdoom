#include "pch.h"
#include "rtdoom.h"
#include "MapDef.h"

using std::vector;
using std::deque;
using std::string;

namespace rtdoom
{
	MapDef::MapDef(const string& mapFolder)
	{
		m_store.Load(mapFolder);

		// open all doors on the map
		std::set<int> doorSectors;
		for (const auto& lineDef : m_store.m_lineDefs)
		{
			if (lineDef.lineType == 1 && lineDef.leftSideDef < 32000)
			{
				const auto& backSide = m_store.m_sideDefs[lineDef.leftSideDef];
				doorSectors.insert(backSide.sector);
			}
		}
		for (auto doorSector : doorSectors)
		{
			m_store.m_sectors[doorSector].ceilingHeight = m_store.m_sectors[doorSector].floorHeight + 72;
		}
	}

	// traverse the BSP tree stored with the map depth-first
	deque<Segment> MapDef::GetSegmentsToDraw(const Point& pov) const
	{
		deque<Segment> segments;
		ProcessNode(pov, *m_store.m_nodes.rbegin(), segments);
		return segments;
	}

	// process tree node and go left or right depending on the locationo of player vs the BSP division line
	void MapDef::ProcessNode(const Point& pov, const MapStore::Node& node, deque<Segment>& segments) const
	{
		if (IsInFrontOf(pov, node))
		{
			ProcessChildRef(node.rightChild, pov, segments);
			ProcessChildRef(node.leftChild, pov, segments);
		}
		else
		{
			ProcessChildRef(node.leftChild, pov, segments);
			ProcessChildRef(node.rightChild, pov, segments);
		}
	}

	// process a child node depending on whether it's an inner node or a leaf (subsector)
	void MapDef::ProcessChildRef(unsigned short childRef, const Point& pov, deque<Segment>& segments) const
	{
		if (childRef & 0x8000)
		{
			// ref is a subsector
			ProcessSubsector(m_store.m_subSectors[childRef & 0x7fff], segments);
		}
		else
		{
			// ref is another node
			ProcessNode(pov, m_store.m_nodes[childRef], segments);
		}
	}

	Thing MapDef::GetStartingPosition() const
	{
		signed short px, py;
		unsigned short pa;
		m_store.GetStartingPosition(px, py, pa);

		Thing player(static_cast<float>(px), static_cast<float>(py), 45, PI2);

		return player;
	}

	bool MapDef::IsInFrontOf(const Point& pov, const Line& line) noexcept
	{
		const auto deltaX = line.e.x - line.s.x;
		const auto deltaY = line.e.y - line.s.y;

		if (deltaX == 0)
		{
			if (pov.x <= line.s.x)
			{
				return deltaY < 0;
			}
			return deltaY > 0;
		}
		if (deltaY == 0)
		{
			if (pov.y <= line.s.y)
			{
				return deltaX > 0;
			}
			return deltaX < 0;
		}

		// use cross-product to determine which side the point is on
		const auto dx = pov.x - line.s.x;
		const auto dy = pov.y - line.s.y;
		const auto left = deltaY * dx;
		const auto right = dy * deltaX;
		return right < left;
	}

	bool MapDef::IsInFrontOf(const Point& pov, const MapStore::Node& node) noexcept
	{
		return IsInFrontOf(pov, Line{
			Vertex{ node.partitionX, node.partitionY },
			Vertex{ node.partitionX + node.deltaX, node.partitionY + node.deltaY } });
	}

	// process serialized map format into usable structures
	void MapDef::ProcessSubsector(const MapStore::SubSector& subSector, deque<Segment>& segments) const
	{
		for (auto i = 0; i < subSector.numSegments; i++)
		{
			const auto& mapSegment = m_store.m_segments[subSector.firstSegment + i];
			const auto& mapStartVertex = m_store.m_vertexes[mapSegment.startVertex];
			const auto& mapEndVertex = m_store.m_vertexes[mapSegment.endVertex];
			const Vertex s{ mapStartVertex.x, mapStartVertex.y };
			const Vertex e{ mapEndVertex.x, mapEndVertex.y };

			bool isSolid = false;
			const auto& lineDef = m_store.m_lineDefs[mapSegment.lineDef];
			Sector frontSector, backSector;
			if (mapSegment.direction == 1 && lineDef.leftSideDef < 32000)
			{
				isSolid = m_store.m_sideDefs[lineDef.leftSideDef].middleTexture[0] != 45 && lineDef.lineType != 1; // open doors

				frontSector = Sector{ m_store.m_sectors[m_store.m_sideDefs[lineDef.leftSideDef].sector] };
				if (lineDef.rightSideDef < 32000)
				{
					backSector = Sector{ m_store.m_sectors[m_store.m_sideDefs[lineDef.rightSideDef].sector] };
				}
			}
			else if (mapSegment.direction == 0 && lineDef.rightSideDef < 32000)
			{
				isSolid = m_store.m_sideDefs[lineDef.rightSideDef].middleTexture[0] != 45 && lineDef.lineType != 1; // open doors

				frontSector = Sector{ m_store.m_sectors[m_store.m_sideDefs[lineDef.rightSideDef].sector] };
				if (lineDef.leftSideDef < 32000)
				{
					backSector = Sector{ m_store.m_sectors[m_store.m_sideDefs[lineDef.leftSideDef].sector] };
				}
			}

			Segment seg{ s, e, isSolid, frontSector, backSector };
			segments.push_back(seg);
		}
	}

	MapDef::~MapDef()
	{
	}
}