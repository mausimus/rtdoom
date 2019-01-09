#include "pch.h"
#include "rtdoom.h"
#include "MapDef.h"

using std::vector;
using std::deque;
using std::string;
using std::shared_ptr;

namespace rtdoom
{
	MapDef::MapDef(const MapStore& mapStore) :
		m_store{ mapStore }
	{
		OpenDoors();
		BuildWireframe();
		BuildSegments();
	}

	MapDef::MapDef(const string& mapFolder)
	{
		m_store.Load(mapFolder);
		OpenDoors();
		BuildWireframe();
		BuildSegments();
	}

	void MapDef::OpenDoors()
	{
		// open all doors on the map
		std::set<int> doorSectors;
		for (const auto& lineDef : m_store.m_lineDefs)
		{
			if ((lineDef.lineType == 1 || lineDef.lineType == 31) && lineDef.leftSideDef < 32000) // TODO: all door linedefs
			{
				const auto& backSide = m_store.m_sideDefs[lineDef.leftSideDef];
				doorSectors.insert(backSide.sector);
			}
		}
		for (auto doorSector : doorSectors)
		{
			m_store.m_sectors[doorSector].ceilingHeight = m_store.m_sectors[doorSector].floorHeight + 72; // TODO: 4 less than the lowest neighbouring sector
		}
	}

	// traverse the BSP tree stored with the map depth-first
	deque<shared_ptr<Segment>> MapDef::GetSegmentsToDraw(const Point& pov) const
	{
		deque<shared_ptr<Segment>> segments;
		ProcessNode(pov, *m_store.m_nodes.rbegin(), segments);
		return segments;
	}

	// find the first sector to draw (POV location)
	std::optional<Sector> MapDef::GetSector(const Point& pov) const
	{
		auto treeIndex = m_store.m_nodes.size() - 1;
		while (true)
		{
			const auto& treeNode = m_store.m_nodes[treeIndex];
			unsigned short childRef = IsInFrontOf(pov, treeNode) ? treeNode.rightChild : treeNode.leftChild;
			if (childRef & 0x8000)
			{
				for (auto i = 0; i < m_store.m_subSectors[childRef & 0x7fff].numSegments; i++)
				{
					const auto& ld = m_store.m_lineDefs[m_store.m_segments[m_store.m_subSectors[childRef & 0x7fff].firstSegment + i].lineDef];
					const bool isInFront = IsInFrontOf(pov, m_store.m_vertexes[ld.startVertex], m_store.m_vertexes[ld.endVertex]);
					if (ld.leftSideDef < 32000 && !isInFront)
					{
						return m_store.m_sectors[m_store.m_sideDefs[ld.leftSideDef].sector];
					}
					else if (ld.rightSideDef < 32000 && isInFront)
					{
						return m_store.m_sectors[m_store.m_sideDefs[ld.rightSideDef].sector];
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
	void MapDef::ProcessNode(const Point& pov, const MapStore::Node& node, deque<shared_ptr<Segment>>& segments) const
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
	void MapDef::ProcessChildRef(unsigned short childRef, const Point& pov, deque<shared_ptr<Segment>>& segments) const
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

		Thing player(static_cast<float>(px), static_cast<float>(py), 0, static_cast<float>(pa / 180.0f * PI));

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
		return IsInFrontOf(pov, Line(
			Vertex{ node.partitionX, node.partitionY },
			Vertex{ node.partitionX + node.deltaX, node.partitionY + node.deltaY }));
	}

	bool MapDef::IsInFrontOf(const Point& pov, const MapStore::Vertex& sv, const MapStore::Vertex& ev) noexcept
	{
		return IsInFrontOf(pov, Line(
			Vertex{ sv.x, sv.y },
			Vertex{ ev.x, ev.y }));
	}

	void MapDef::BuildWireframe()
	{
		for (const auto& l : m_store.m_lineDefs)
		{
			const auto& sv = m_store.m_vertexes[l.startVertex];
			const auto& ev = m_store.m_vertexes[l.endVertex];
			m_wireframe.push_back(Line(Vertex(sv.x, sv.y), Vertex(ev.x, ev.y)));
		}
	}

	void MapDef::BuildSegments()
	{
		m_segments.reserve(m_store.m_segments.size());
		for (const auto& mapSegment : m_store.m_segments)
		{
			const auto& mapStartVertex = m_store.m_vertexes[mapSegment.startVertex];
			const auto& mapEndVertex = m_store.m_vertexes[mapSegment.endVertex];
			const Vertex s{ mapStartVertex.x, mapStartVertex.y };
			const Vertex e{ mapEndVertex.x, mapEndVertex.y };

			bool isSolid = false;
			const auto& lineDef = m_store.m_lineDefs[mapSegment.lineDef];
			Sector frontSector, backSector;
			MapStore::SideDef frontSide, backSide;
			if (mapSegment.direction == 1 && lineDef.leftSideDef < 32000)
			{
				isSolid = lineDef.leftSideDef > 32000;
				frontSide = m_store.m_sideDefs[lineDef.leftSideDef];
				frontSector = Sector{ m_store.m_sectors[frontSide.sector] };
				if (lineDef.rightSideDef < 32000)
				{
					backSide = m_store.m_sideDefs[lineDef.rightSideDef];
					backSector = Sector{ m_store.m_sectors[backSide.sector] };
				}
			}
			else if (mapSegment.direction == 0 && lineDef.rightSideDef < 32000)
			{
				isSolid = lineDef.leftSideDef > 32000;
				frontSide = m_store.m_sideDefs[lineDef.rightSideDef];
				frontSector = Sector{ m_store.m_sectors[frontSide.sector] };
				if (lineDef.leftSideDef < 32000)
				{
					backSide = m_store.m_sideDefs[lineDef.leftSideDef];
					backSector = Sector{ m_store.m_sectors[backSide.sector] };
				}
			}

			Side front{ frontSector, Helpers::MakeString(frontSide.lowerTexture), Helpers::MakeString(frontSide.middleTexture), Helpers::MakeString(frontSide.upperTexture), frontSide.xOffset, frontSide.yOffset };
			Side back{ backSector, Helpers::MakeString(backSide.lowerTexture), Helpers::MakeString(backSide.middleTexture), Helpers::MakeString(backSide.upperTexture), frontSide.xOffset, frontSide.yOffset };

			m_segments.push_back(std::make_shared<Segment>(s, e, isSolid, front, back, mapSegment.offset, (bool)(lineDef.flags & 0x0010), (bool)(lineDef.flags & 0x0008)));
		}
	}

	// process serialized map format into usable structures
	void MapDef::ProcessSubsector(const MapStore::SubSector& subSector, deque<shared_ptr<Segment>>& segments) const
	{
		for (auto i = 0; i < subSector.numSegments; i++)
		{
			segments.push_back(m_segments[subSector.firstSegment + i]);
		}
	}

	MapDef::~MapDef()
	{
	}
}